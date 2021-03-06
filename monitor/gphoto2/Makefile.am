
NULL =

libexec_PROGRAMS = gvfs-gphoto2-volume-monitor

BUILT_SOURCES =                                         \
	hal-marshal.h           hal-marshal.c

hal-marshal.h: hal-marshal.list
	$(AM_V_GEN) glib-genmarshal $< --prefix=hal_marshal --header > $@

hal-marshal.c: hal-marshal.list
	$(AM_V_GEN) echo "#include \"hal-marshal.h\"" > $@ && glib-genmarshal $< --prefix=hal_marshal --body >> $@


if USE_GUDEV
gvfs_gphoto2_volume_monitor_SOURCES =

else
gvfs_gphoto2_volume_monitor_SOURCES =			\
	hal-utils.c 		hal-utils.h 		\
	hal-marshal.c		hal-marshal.h		\
	hal-device.c		hal-device.h		\
	hal-pool.c		hal-pool.h		\
	$(NULL)
endif

gvfs_gphoto2_volume_monitor_SOURCES +=			\
	gphoto2-volume-monitor-daemon.c			\
	ggphoto2volume.c	ggphoto2volume.h	\
	ggphoto2volumemonitor.c	ggphoto2volumemonitor.h	\
	$(NULL)

gvfs_gphoto2_volume_monitor_CFLAGS =		\
	-DG_LOG_DOMAIN=\"GVFS-GPhoto2\"		\
	-I$(top_srcdir)/common                  \
	-I$(top_srcdir)/monitor/proxy           \
	$(GLIB_CFLAGS)                          \
	$(GPHOTO2_CFLAGS)			\
	-DGIO_MODULE_DIR=\"$(GIO_MODULE_DIR)\"	\
	-DGVFS_LOCALEDIR=\""$(localedir)"\"	\
	$(NULL)

if USE_GUDEV
gvfs_gphoto2_volume_monitor_CFLAGS += $(GUDEV_CFLAGS)
else
gvfs_gphoto2_volume_monitor_CFLAGS += $(HAL_CFLAGS)
endif

gvfs_gphoto2_volume_monitor_LDFLAGS =	\
	$(NULL)

gvfs_gphoto2_volume_monitor_LDADD  =		     			      \
	$(GLIB_LIBS)                                 			      \
	$(GPHOTO2_LIBS)                              			      \
	$(top_builddir)/common/libgvfscommon.la 			      \
	$(top_builddir)/monitor/proxy/libgvfsproxyvolumemonitordaemon-noin.la \
	$(NULL)

if USE_GUDEV
gvfs_gphoto2_volume_monitor_LDADD +=                                          \
	$(GUDEV_LIBS)                                                         \
	$(top_builddir)/common/libgvfscommon-gphoto2.la                       \
	$(NULL)
else
gvfs_gphoto2_volume_monitor_LDADD +=                                          \
	$(HAL_LIBS)                                                          \
	$(top_builddir)/common/libgvfscommon-hal.la                          \
	$(NULL)
endif


remote_volume_monitorsdir = $(datadir)/gvfs/remote-volume-monitors
remote_volume_monitors_DATA = gphoto2.monitor

servicedir       = $(datadir)/dbus-1/services
service_in_files = org.gtk.vfs.GPhoto2VolumeMonitor.service.in
service_DATA     = $(service_in_files:.service.in=.service)

systemd_user_in_files = gvfs-gphoto2-volume-monitor.service.in
if USE_SYSTEMD_USER_UNITS
systemd_user_DATA = $(systemd_user_in_files:.service.in=.service)
endif

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN) $(SED) -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

$(systemd_user_DATA): $(systemd_user_in_files) Makefile
	$(AM_V_GEN) $(SED) -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

CLEANFILES = \
	*~ \
	*.loT \
	$(BUILT_SOURCES) \
	$(service_DATA) \
	$(systemd_user_DATA) \
	$(NULL)

EXTRA_DIST = \
	hal-marshal.list \
	$(service_in_files) \
	$(systemd_user_in_files) \
	$(remote_volume_monitors_DATA) \
	$(NULL)
