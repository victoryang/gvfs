
NULL =

libexec_PROGRAMS = gvfs-storaged-volume-monitor

gvfs_storaged_volume_monitor_SOURCES = \
	storagedvolumemonitordaemon.c \
	gvfsstoragedvolumemonitor.c gvfsstoragedvolumemonitor.h \
	gvfsstorageddrive.c gvfsstorageddrive.h \
	gvfsstoragedvolume.c gvfsstoragedvolume.h \
	gvfsstoragedmount.c gvfsstoragedmount.h \
	gvfsstoragedutils.c gvfsstoragedutils.h \
	$(NULL)

gvfs_storaged_volume_monitor_CFLAGS = \
	-DG_LOG_DOMAIN=\"GVFS-Storaged\" \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/monitor/proxy \
	$(GLIB_CFLAGS) \
	$(STORAGED_CFLAGS) \
	$(GUDEV_CFLAGS) \
	$(LIBSYSTEMD_LOGIN_CFLAGS) \
	$(KEYRING_CFLAGS) \
	-DGIO_MODULE_DIR=\"$(GIO_MODULE_DIR)\" \
	-DGVFS_LOCALEDIR=\""$(localedir)"\" \
	-DG_DISABLE_DEPRECATED \
	$(NULL)

gvfs_storaged_volume_monitor_LDFLAGS = \
	$(NULL)

gvfs_storaged_volume_monitor_LDADD = \
	$(GLIB_LIBS) \
	$(STORAGED_LIBS) \
	$(GUDEV_LIBS) \
	$(LIBSYSTEMD_LOGIN_LIBS) \
	$(KEYRING_LIBS) \
	$(top_builddir)/common/libgvfscommon.la \
	$(top_builddir)/common/libgvfscommon-monitor.la \
	$(top_builddir)/monitor/proxy/libgvfsproxyvolumemonitordaemon-noin.la \
	$(NULL)

remote_volume_monitorsdir = $(datadir)/gvfs/remote-volume-monitors
remote_volume_monitors_DATA = storaged.monitor

servicedir = $(datadir)/dbus-1/services
service_in_files = org.gtk.vfs.StoragedVolumeMonitor.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN) $(SED) -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

clean-local:
	rm -f *~ *.loT $(BUILT_SOURCES) $(service_DATA)

EXTRA_DIST = $(service_in_files) storaged.monitor
