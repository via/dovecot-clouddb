pkglibexecdir = $(libexecdir)/dovecot

pkglibexec_PROGRAMS = \
	rawlog \
	script \
	script-login \
	$(TCPWRAP_BIN) \
	gdbhelper \
	listview \
	maildirlock

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/lib \
	-I$(top_srcdir)/src/lib-auth \
	-I$(top_srcdir)/src/lib-dict \
	-I$(top_srcdir)/src/lib-settings \
	-I$(top_srcdir)/src/lib-master \
	-I$(top_srcdir)/src/lib-mail \
	-I$(top_srcdir)/src/lib-imap \
	-I$(top_srcdir)/src/lib-index \
	-I$(top_srcdir)/src/lib-storage \
	-I$(top_srcdir)/src/auth \
	-DPKG_LIBEXECDIR=\""$(pkglibexecdir)"\" \
	-DPKG_RUNDIR=\""$(rundir)"\"

rawlog_LDADD = $(LIBDOVECOT)
rawlog_DEPENDENCIES = $(LIBDOVECOT_DEPS)
rawlog_SOURCES = \
	rawlog.c

script_login_LDADD = $(LIBDOVECOT_STORAGE) $(LIBDOVECOT) $(MODULE_LIBS)
script_login_DEPENDENCIES = $(LIBDOVECOT_STORAGE) $(LIBDOVECOT_DEPS)
script_login_SOURCES = \
	script-login.c

script_LDADD = $(LIBDOVECOT) $(MODULE_LIBS)
script_DEPENDENCIES = $(LIBDOVECOT_DEPS)
script_SOURCES = \
	script.c

if TCPWRAPPERS
TCPWRAP_BIN = tcpwrap
tcpwrap_LDADD = $(LIBDOVECOT) $(MODULE_LIBS) $(LIBWRAP_LIBS)
tcpwrap_DEPENDENCIES = $(LIBDOVECOT_DEPS)
tcpwrap_SOURCES = \
	tcpwrap.c \
	tcpwrap-settings.c
endif

gdbhelper_LDADD = $(LIBDOVECOT)
gdbhelper_DEPENDENCIES = $(LIBDOVECOT_DEPS)
gdbhelper_SOURCES = \
	gdbhelper.c

listview_LDADD = $(LIBDOVECOT)
listview_DEPENDENCIES = $(LIBDOVECOT_DEPS)
listview_SOURCES = \
	listview.c

maildirlock_LDADD = $(LIBDOVECOT)
maildirlock_DEPENDENCIES = $(LIBDOVECOT_DEPS)
maildirlock_SOURCES = \
	maildirlock.c
