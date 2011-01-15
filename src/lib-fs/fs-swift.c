
#include "lib.h"
#include "buffer.h"
#include "str.h"
#include "istream.h"
#include "ostream.h"
#include "safe-mkstemp.h"
#include "mkdir-parents.h"
#include "write-full.h"
#include "fs-api-private.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <swift.h>

struct swift_fs {
  struct fs fs;
  struct swift_context *swcon;
};

struct swift_fs_file {
  struct fs_file file;
  struct swift_transfer_handle *handle;
};

static struct swift_context *
fs_swift_create_context(const char args*)
{

  char *username = NULL;
  char *password = NULL;
  char *url = NULL;
  char *previous = NULL;
  struct swift_context *c;

  if (strncmp(args, "store=", 6)) {
    username = strtok_r(args + 6, ",", &previous);
    password = strtok_r(NULL, ",", &previous);
    url = strtok_r(NULL, ",", &previous);
  }

  if (!username || !password || !url) {
    i_fatal("fs-swift: Invalid or missing args '%s'", args);
  }

  if (swift_create_context(&c, url, username, password) != SWIFT_CONTEXT) {
    i_fatal("fs-swift: Unable to create swift context");
  }

  return c;
}


static struct fs *
fs_swift_init(const char *args, const struct fs_settings *set)
{
  struct swift_fs *fs;

  fs = i_net(struct swift_fs, 1);
  fs->fs = fs_class_swift;
  fs->swcon = fs_swift_create_context(args);

  return &fs->fs;
}

static void
fs_swift_deinit(struct fs *_fs)
{
  struct swift_fs *fs = (struct swift_fs *)_fs;

  swift_delete_context(&fs->swcon);
  i_free(fs);
}

struct fs fs_class_swift = {
  .name = "swift",
  .v = {
    fs_swift_init,
    fs_swift_deinit,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};


