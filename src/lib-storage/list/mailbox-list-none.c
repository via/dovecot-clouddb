/* Copyright (c) 2006-2010 Dovecot authors, see the included COPYING file */

#include "lib.h"
#include "mailbox-list-private.h"

#define MAILBOX_LIST_NAME_NONE "none"
#define GLOBAL_TEMP_PREFIX ".temp."

extern struct mailbox_list none_mailbox_list;

static struct mailbox_list *none_list_alloc(void)
{
	struct mailbox_list *list;
	pool_t pool;

	pool = pool_alloconly_create("none list", 2048);

	list = p_new(pool, struct mailbox_list, 1);
	*list = none_mailbox_list;
	list->pool = pool;
	return list;
}

static void none_list_deinit(struct mailbox_list *list)
{
	pool_unref(&list->pool);
}

static bool
none_is_valid_pattern(struct mailbox_list *list ATTR_UNUSED,
		      const char *pattern ATTR_UNUSED)
{
	return TRUE;
}

static bool
none_is_valid_existing_name(struct mailbox_list *list ATTR_UNUSED,
			    const char *name ATTR_UNUSED)
{
	return TRUE;
}

static bool
none_is_valid_create_name(struct mailbox_list *list ATTR_UNUSED,
			  const char *name ATTR_UNUSED)
{
	return FALSE;
}

static const char *
none_list_get_path(struct mailbox_list *list ATTR_UNUSED,
		   const char *name ATTR_UNUSED,
		   enum mailbox_list_path_type type ATTR_UNUSED)
{
	if (type == MAILBOX_LIST_PATH_TYPE_INDEX)
		return "";
	return NULL;
}

static int
none_list_get_mailbox_name_status(struct mailbox_list *list ATTR_UNUSED,
				  const char *name ATTR_UNUSED,
				  enum mailbox_name_status *status)
{
	*status = MAILBOX_NAME_VALID;
	return 0;
}

static const char *
none_list_get_temp_prefix(struct mailbox_list *list ATTR_UNUSED,
			  bool global ATTR_UNUSED)
{
	return GLOBAL_TEMP_PREFIX;
}

static int none_list_set_subscribed(struct mailbox_list *list,
				    const char *name ATTR_UNUSED,
				    bool set ATTR_UNUSED)
{
	mailbox_list_set_error(list, MAIL_ERROR_NOTPOSSIBLE, "Not supported");
	return -1;
}

static int
none_list_create_mailbox_dir(struct mailbox_list *list,
			     const char *name ATTR_UNUSED,
			     enum mailbox_dir_create_type type ATTR_UNUSED)
{
	mailbox_list_set_error(list, MAIL_ERROR_NOTPOSSIBLE, "Not supported");
	return -1;
}

static int none_list_delete_mailbox(struct mailbox_list *list,
				    const char *name ATTR_UNUSED)
{
	mailbox_list_set_error(list, MAIL_ERROR_NOTPOSSIBLE, "Not supported");
	return -1;
}

static int none_list_delete_dir(struct mailbox_list *list,
				const char *name ATTR_UNUSED)
{
	mailbox_list_set_error(list, MAIL_ERROR_NOTPOSSIBLE, "Not supported");
	return -1;
}

static int
none_list_rename_mailbox(struct mailbox_list *oldlist,
			 const char *oldname ATTR_UNUSED,
			 struct mailbox_list *newlist ATTR_UNUSED,
			 const char *newname ATTR_UNUSED,
			 bool rename_children ATTR_UNUSED)
{
	mailbox_list_set_error(oldlist, MAIL_ERROR_NOTPOSSIBLE,
			       "Not supported");
	return -1;
}

static struct mailbox_list_iterate_context *
none_list_iter_init(struct mailbox_list *list,
		    const char *const *patterns ATTR_UNUSED,
		    enum mailbox_list_iter_flags flags)
{
	struct mailbox_list_iterate_context *ctx;

	ctx = i_new(struct mailbox_list_iterate_context, 1);
	ctx->list = list;
	ctx->flags = flags;
	return ctx;
}

static int
none_list_iter_deinit(struct mailbox_list_iterate_context *ctx)
{
	i_free(ctx);
	return 0;
}

static const struct mailbox_info *
none_list_iter_next(struct mailbox_list_iterate_context *ctx ATTR_UNUSED)
{
	return NULL;
}

static int
none_list_get_mailbox_flags(struct mailbox_list *list ATTR_UNUSED,
			    const char *dir ATTR_UNUSED,
			    const char *fname ATTR_UNUSED,
			    enum mailbox_list_file_type type ATTR_UNUSED,
			    struct stat *st_r ATTR_UNUSED,
			    enum mailbox_info_flags *flags)
{
	*flags = MAILBOX_NONEXISTENT;
	return 0;
}

struct mailbox_list none_mailbox_list = {
	.name = MAILBOX_LIST_NAME_NONE,
	.hierarchy_sep = '/',
	.props = MAILBOX_LIST_PROP_NO_ROOT,
	.mailbox_name_max_length = MAILBOX_LIST_NAME_MAX_LENGTH,

	{
		none_list_alloc,
		none_list_deinit,
		NULL,
		none_is_valid_pattern,
		none_is_valid_existing_name,
		none_is_valid_create_name,
		none_list_get_path,
		none_list_get_mailbox_name_status,
		none_list_get_temp_prefix,
		NULL,
		none_list_iter_init,
		none_list_iter_next,
		none_list_iter_deinit,
		none_list_get_mailbox_flags,
		NULL,
		none_list_set_subscribed,
		none_list_create_mailbox_dir,
		none_list_delete_mailbox,
		none_list_delete_dir,
		none_list_rename_mailbox
	}
};
