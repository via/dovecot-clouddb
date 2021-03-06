/* Copyright (c) 2002-2010 Dovecot authors, see the included COPYING file */

#include "imap-common.h"
#include "array.h"
#include "buffer.h"
#include "imap-commands.h"

#include <stdlib.h>

static const struct command imap4rev1_commands[] = {
	{ "CAPABILITY",		cmd_capability,  0 },
	{ "LOGOUT",		cmd_logout,      COMMAND_FLAG_BREAKS_MAILBOX },
	{ "NOOP",		cmd_noop,        COMMAND_FLAG_BREAKS_SEQS },

	{ "APPEND",		cmd_append,      COMMAND_FLAG_BREAKS_SEQS },
	{ "EXAMINE",		cmd_examine,     COMMAND_FLAG_BREAKS_MAILBOX },
	{ "CREATE",		cmd_create,      0 },
	{ "DELETE",		cmd_delete,      COMMAND_FLAG_BREAKS_MAILBOX |
						 COMMAND_FLAG_USE_NONEXISTENT },
	{ "RENAME",		cmd_rename,      COMMAND_FLAG_USE_NONEXISTENT },
	{ "LIST",		cmd_list,        0 },
	{ "LSUB",		cmd_lsub,        0 },
	{ "SELECT",		cmd_select,      COMMAND_FLAG_BREAKS_MAILBOX },
	{ "STATUS",		cmd_status,      0 },
	{ "SUBSCRIBE",		cmd_subscribe,   0 },
	{ "UNSUBSCRIBE",	cmd_unsubscribe, COMMAND_FLAG_USE_NONEXISTENT },

	{ "CHECK",		cmd_check,       COMMAND_FLAG_BREAKS_SEQS },
	{ "CLOSE",		cmd_close,       COMMAND_FLAG_BREAKS_MAILBOX },
	{ "COPY",		cmd_copy,        COMMAND_FLAG_USES_SEQS |
						 COMMAND_FLAG_BREAKS_SEQS },
	{ "EXPUNGE",		cmd_expunge,     COMMAND_FLAG_BREAKS_SEQS },
	{ "FETCH",		cmd_fetch,       COMMAND_FLAG_USES_SEQS },
	{ "SEARCH",		cmd_search,      COMMAND_FLAG_USES_SEQS },
	{ "STORE",		cmd_store,       COMMAND_FLAG_USES_SEQS },
	{ "UID",		cmd_uid,         0 },
	{ "UID COPY",		cmd_copy,        COMMAND_FLAG_BREAKS_SEQS },
	{ "UID FETCH",		cmd_fetch,       COMMAND_FLAG_BREAKS_SEQS },
	{ "UID SEARCH",		cmd_search,      COMMAND_FLAG_BREAKS_SEQS },
	{ "UID STORE",		cmd_store,       COMMAND_FLAG_BREAKS_SEQS }
};
#define IMAP4REV1_COMMANDS_COUNT N_ELEMENTS(imap4rev1_commands)

static const struct command imap_ext_commands[] = {
	{ "CANCELUPDATE",	cmd_cancelupdate,0 },
	{ "ENABLE",		cmd_enable,      0 },
	{ "ID",			cmd_id,          0 },
	{ "IDLE",		cmd_idle,        COMMAND_FLAG_BREAKS_SEQS |
						 COMMAND_FLAG_REQUIRES_SYNC },
	{ "NAMESPACE",		cmd_namespace,   0 },
	{ "SORT",		cmd_sort,        COMMAND_FLAG_USES_SEQS },
	{ "THREAD",		cmd_thread,      COMMAND_FLAG_USES_SEQS },
	{ "UID EXPUNGE",	cmd_uid_expunge, COMMAND_FLAG_BREAKS_SEQS },
	{ "UID SORT",		cmd_sort,        COMMAND_FLAG_BREAKS_SEQS },
	{ "UID THREAD",		cmd_thread,      COMMAND_FLAG_BREAKS_SEQS },
	{ "UNSELECT",		cmd_unselect,    COMMAND_FLAG_BREAKS_MAILBOX },
	{ "X-CANCEL",		cmd_x_cancel,    0 }
};
#define IMAP_EXT_COMMANDS_COUNT N_ELEMENTS(imap_ext_commands)

ARRAY_TYPE(command) imap_commands;
static bool commands_unsorted;

void command_register(const char *name, command_func_t *func,
		      enum command_flags flags)
{
	struct command cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.name = name;
	cmd.func = func;
	cmd.flags = flags;
	array_append(&imap_commands, &cmd, 1);

	commands_unsorted = TRUE;
}

void command_unregister(const char *name)
{
	const struct command *cmd;
	unsigned int i, count;

	cmd = array_get(&imap_commands, &count);
	for (i = 0; i < count; i++) {
		if (strcasecmp(cmd[i].name, name) == 0) {
			array_delete(&imap_commands, i, 1);
			return;
		}
	}

	i_error("Trying to unregister unknown command '%s'", name);
}

void command_register_array(const struct command *cmdarr, unsigned int count)
{
	commands_unsorted = TRUE;
	array_append(&imap_commands, cmdarr, count);
}

void command_unregister_array(const struct command *cmdarr, unsigned int count)
{
	while (count > 0) {
		command_unregister(cmdarr->name);
		count--; cmdarr++;
	}
}

static int command_cmp(const struct command *c1, const struct command *c2)
{
	return strcasecmp(c1->name, c2->name);
}

static int command_bsearch(const char *name, const struct command *cmd)
{
	return strcasecmp(name, cmd->name);
}

struct command *command_find(const char *name)
{
	if (commands_unsorted) {
		array_sort(&imap_commands, command_cmp);
                commands_unsorted = FALSE;
	}

	return array_bsearch(&imap_commands, name, command_bsearch);
}

void commands_init(void)
{
	i_array_init(&imap_commands, 64);
	commands_unsorted = FALSE;

        command_register_array(imap4rev1_commands, IMAP4REV1_COMMANDS_COUNT);
        command_register_array(imap_ext_commands, IMAP_EXT_COMMANDS_COUNT);
}

void commands_deinit(void)
{
        command_unregister_array(imap4rev1_commands, IMAP4REV1_COMMANDS_COUNT);
        command_unregister_array(imap_ext_commands, IMAP_EXT_COMMANDS_COUNT);
	array_free(&imap_commands);
}
