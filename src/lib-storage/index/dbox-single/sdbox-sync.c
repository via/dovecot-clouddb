/* Copyright (c) 2007-2010 Dovecot authors, see the included COPYING file */

#include "lib.h"
#include "dbox-attachment.h"
#include "sdbox-storage.h"
#include "sdbox-file.h"
#include "sdbox-sync.h"

#define SDBOX_REBUILD_COUNT 3

static void
dbox_sync_file_move_if_needed(struct dbox_file *file,
			      enum sdbox_sync_entry_type type)
{
	bool move_to_alt = type == SDBOX_SYNC_ENTRY_TYPE_MOVE_TO_ALT;
	bool deleted;

	if (move_to_alt != dbox_file_is_in_alt(file)) {
		/* move the file. if it fails, nothing broke so
		   don't worry about it. */
		if (dbox_file_open(file, &deleted) > 0 && !deleted)
			(void)sdbox_file_move(file, move_to_alt);
	}
}

static void dbox_sync_file_expunge(struct sdbox_sync_context *ctx,
				   struct dbox_file *file, uint32_t seq)
{
	struct sdbox_file *sfile = (struct sdbox_file *)file;
	struct mailbox *box = &ctx->mbox->box;
	int ret;

	if (mail_index_transaction_is_expunged(ctx->trans, seq)) {
		/* already expunged within this transaction */
		return;
	}

	if (file->storage->attachment_dir != NULL)
		ret = sdbox_file_unlink_with_attachments(sfile);
	else
		ret = dbox_file_unlink(file);
	if (ret < 0) {
		/* some non-ENOENT error trying to unlink the file */
		return;
	}

	/* file was either unlinked by us or someone else */
	mail_index_expunge(ctx->trans, seq);
	if (box->v.sync_notify != NULL)
		box->v.sync_notify(box, sfile->uid, MAILBOX_SYNC_TYPE_EXPUNGE);
}

static void sdbox_sync_file(struct sdbox_sync_context *ctx,
			    uint32_t seq, uint32_t uid,
			    enum sdbox_sync_entry_type type)
{
	struct dbox_file *file;

	file = sdbox_file_init(ctx->mbox, uid);
	switch (type) {
	case SDBOX_SYNC_ENTRY_TYPE_EXPUNGE:
		dbox_sync_file_expunge(ctx, file, seq);
		break;
	case SDBOX_SYNC_ENTRY_TYPE_MOVE_FROM_ALT:
	case SDBOX_SYNC_ENTRY_TYPE_MOVE_TO_ALT:
		dbox_sync_file_move_if_needed(file, type);
		break;
	}
	dbox_file_unref(&file);
}

static void sdbox_sync_add(struct sdbox_sync_context *ctx,
			   const struct mail_index_sync_rec *sync_rec)
{
	uint32_t uid;
	enum sdbox_sync_entry_type type;
	uint32_t seq, seq1, seq2;

	if (sync_rec->type == MAIL_INDEX_SYNC_TYPE_EXPUNGE) {
		/* we're interested */
		type = SDBOX_SYNC_ENTRY_TYPE_EXPUNGE;
	} else if (sync_rec->type == MAIL_INDEX_SYNC_TYPE_FLAGS) {
		/* we care only about alt flag changes */
		if ((sync_rec->add_flags & DBOX_INDEX_FLAG_ALT) != 0)
			type = SDBOX_SYNC_ENTRY_TYPE_MOVE_TO_ALT;
		else if ((sync_rec->remove_flags & DBOX_INDEX_FLAG_ALT) != 0)
			type = SDBOX_SYNC_ENTRY_TYPE_MOVE_FROM_ALT;
		else
			return;
	} else {
		/* not interested */
		return;
	}

	if (!mail_index_lookup_seq_range(ctx->sync_view,
					 sync_rec->uid1, sync_rec->uid2,
					 &seq1, &seq2)) {
		/* already expunged everything. nothing to do. */
		return;
	}

	for (seq = seq1; seq <= seq2; seq++) {
		mail_index_lookup_uid(ctx->sync_view, seq, &uid);
		sdbox_sync_file(ctx, seq, uid, type);
	}
}

static int sdbox_sync_index(struct sdbox_sync_context *ctx)
{
	struct mailbox *box = &ctx->mbox->box;
	const struct mail_index_header *hdr;
	struct mail_index_sync_rec sync_rec;
	uint32_t seq1, seq2;

	hdr = mail_index_get_header(ctx->sync_view);
	if (hdr->uid_validity == 0) {
		/* newly created index file */
		return 0;
	}

	/* mark the newly seen messages as recent */
	if (mail_index_lookup_seq_range(ctx->sync_view, hdr->first_recent_uid,
					hdr->next_uid, &seq1, &seq2))
		index_mailbox_set_recent_seq(box, ctx->sync_view, seq1, seq2);

	while (mail_index_sync_next(ctx->index_sync_ctx, &sync_rec))
		sdbox_sync_add(ctx, &sync_rec);

	if (box->v.sync_notify != NULL)
		box->v.sync_notify(box, 0, 0);
	return 1;
}

static int
sdbox_refresh_header(struct sdbox_mailbox *mbox, bool retry, bool log_error)
{
	struct mail_index_view *view;
	struct sdbox_index_header hdr;
	int ret;

	view = mail_index_view_open(mbox->box.index);
	ret = sdbox_read_header(mbox, &hdr, log_error);
	mail_index_view_close(&view);

	if (ret < 0 && retry) {
		(void)mail_index_refresh(mbox->box.index);
		return sdbox_refresh_header(mbox, FALSE, log_error);
	}
	return ret;
}

int sdbox_sync_begin(struct sdbox_mailbox *mbox, enum sdbox_sync_flags flags,
		     struct sdbox_sync_context **ctx_r)
{
	struct mail_storage *storage = mbox->box.storage;
	struct sdbox_sync_context *ctx;
	enum mail_index_sync_flags sync_flags;
	unsigned int i;
	int ret;
	bool rebuild, force_rebuild;

	force_rebuild = (flags & SDBOX_SYNC_FLAG_FORCE_REBUILD) != 0;
	rebuild = force_rebuild ||
		mbox->corrupted_rebuild_count != 0 ||
		sdbox_refresh_header(mbox, TRUE, FALSE) < 0;

	ctx = i_new(struct sdbox_sync_context, 1);
	ctx->mbox = mbox;
	ctx->flags = flags;

	sync_flags = index_storage_get_sync_flags(&mbox->box);
	if (!rebuild && (flags & SDBOX_SYNC_FLAG_FORCE) == 0)
		sync_flags |= MAIL_INDEX_SYNC_FLAG_REQUIRE_CHANGES;
	if ((flags & SDBOX_SYNC_FLAG_FSYNC) != 0)
		sync_flags |= MAIL_INDEX_SYNC_FLAG_FSYNC;
	/* don't write unnecessary dirty flag updates */
	sync_flags |= MAIL_INDEX_SYNC_FLAG_AVOID_FLAG_UPDATES;

	for (i = 0;; i++) {
		ret = mail_index_sync_begin(mbox->box.index,
					    &ctx->index_sync_ctx,
					    &ctx->sync_view, &ctx->trans,
					    sync_flags);
		if (ret <= 0) {
			if (ret < 0)
				mail_storage_set_index_error(&mbox->box);
			i_free(ctx);
			*ctx_r = NULL;
			return ret;
		}

		if (rebuild)
			ret = 0;
		else {
			if ((ret = sdbox_sync_index(ctx)) > 0)
				break;
		}

		/* failure. keep the index locked while we're doing a
		   rebuild. */
		if (ret == 0) {
			if (i >= SDBOX_REBUILD_COUNT) {
				mail_storage_set_critical(storage,
					"sdbox %s: Index keeps breaking",
					ctx->mbox->box.path);
				ret = -1;
			} else {
				/* do a full resync and try again. */
				i_warning("sdbox %s: Rebuilding index",
					  ctx->mbox->box.path);
				rebuild = FALSE;
				ret = sdbox_sync_index_rebuild(mbox,
							       force_rebuild);
			}
		}
		mail_index_sync_rollback(&ctx->index_sync_ctx);
		if (ret < 0) {
			i_free(ctx);
			return -1;
		}
	}

	*ctx_r = ctx;
	return 0;
}

int sdbox_sync_finish(struct sdbox_sync_context **_ctx, bool success)
{
	struct sdbox_sync_context *ctx = *_ctx;
	int ret = success ? 0 : -1;

	*_ctx = NULL;

	if (success) {
		if (mail_index_sync_commit(&ctx->index_sync_ctx) < 0) {
			mail_storage_set_index_error(&ctx->mbox->box);
			ret = -1;
		}
	} else {
		mail_index_sync_rollback(&ctx->index_sync_ctx);
	}

	i_free(ctx);
	return ret;
}

int sdbox_sync(struct sdbox_mailbox *mbox, enum sdbox_sync_flags flags)
{
	struct sdbox_sync_context *sync_ctx;

	if (sdbox_sync_begin(mbox, flags, &sync_ctx) < 0)
		return -1;

	if (sync_ctx == NULL)
		return 0;
	return sdbox_sync_finish(&sync_ctx, TRUE);
}

struct mailbox_sync_context *
sdbox_storage_sync_init(struct mailbox *box, enum mailbox_sync_flags flags)
{
	struct sdbox_mailbox *mbox = (struct sdbox_mailbox *)box;
	enum sdbox_sync_flags sdbox_sync_flags = 0;
	int ret = 0;

	if (!box->opened) {
		if (mailbox_open(box) < 0)
			ret = -1;
	}

	if (ret == 0 && index_mailbox_want_full_sync(&mbox->box, flags)) {
		if ((flags & MAILBOX_SYNC_FLAG_FORCE_RESYNC) != 0)
			sdbox_sync_flags |= SDBOX_SYNC_FLAG_FORCE_REBUILD;
		ret = sdbox_sync(mbox, sdbox_sync_flags);
	}

	return index_mailbox_sync_init(box, flags, ret < 0);
}
