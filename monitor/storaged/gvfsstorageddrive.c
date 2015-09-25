/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/* gvfs - extensions for gio
 *
 * Copyright (C) 2006-2012 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Author: David Zeuthen <davidz@redhat.com>
 */

#include <config.h>

#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "gvfsstoragedvolumemonitor.h"
#include "gvfsstorageddrive.h"
#include "gvfsstoragedvolume.h"
#include "gvfsstoragedutils.h"

typedef struct _GVfsStoragedDriveClass GVfsStoragedDriveClass;

struct _GVfsStoragedDriveClass
{
  GObjectClass parent_class;
};

struct _GVfsStoragedDrive
{
  GObject parent;

  GVfsStoragedVolumeMonitor  *monitor; /* owned by volume monitor */
  GList                     *volumes; /* entries in list are owned by volume monitor */

  /* If TRUE, the drive was discovered at coldplug time */
  gboolean coldplug;

  StoragedDrive *storaged_drive;

  GIcon *icon;
  GIcon *symbolic_icon;
  gchar *name;
  gchar *sort_key;
  gchar *device_file;
  dev_t dev;
  gboolean is_media_removable;
  gboolean has_media;
  gboolean can_eject;
  gboolean can_stop;
};

static void gvfs_storaged_drive_drive_iface_init (GDriveIface *iface);

static void on_storaged_drive_notify (GObject     *object,
                                    GParamSpec  *pspec,
                                    gpointer     user_data);

G_DEFINE_TYPE_EXTENDED (GVfsStoragedDrive, gvfs_storaged_drive, G_TYPE_OBJECT, 0,
                        G_IMPLEMENT_INTERFACE (G_TYPE_DRIVE, gvfs_storaged_drive_drive_iface_init))

static void
gvfs_storaged_drive_finalize (GObject *object)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (object);
  GList *l;

  for (l = drive->volumes; l != NULL; l = l->next)
    {
      GVfsStoragedVolume *volume = l->data;
      gvfs_storaged_volume_unset_drive (volume, drive);
    }

  if (drive->storaged_drive != NULL)
    {
      g_signal_handlers_disconnect_by_func (drive->storaged_drive, on_storaged_drive_notify, drive);
      g_object_unref (drive->storaged_drive);
    }

  g_clear_object (&drive->icon);
  g_clear_object (&drive->symbolic_icon);
  g_free (drive->name);
  g_free (drive->sort_key);
  g_free (drive->device_file);

  G_OBJECT_CLASS (gvfs_storaged_drive_parent_class)->finalize (object);
}

static void
gvfs_storaged_drive_class_init (GVfsStoragedDriveClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = gvfs_storaged_drive_finalize;
}

static void
gvfs_storaged_drive_init (GVfsStoragedDrive *gdu_drive)
{
}

static void
emit_changed (GVfsStoragedDrive *drive)
{
  g_signal_emit_by_name (drive, "changed");
  g_signal_emit_by_name (drive->monitor, "drive-changed", drive);
}


#if STORAGED_CHECK_VERSION(2,0,90)
static gpointer
_g_object_ref0 (gpointer object)
{
  if (object != NULL)
    return g_object_ref (G_OBJECT (object));
  else
    return NULL;
}
#endif

static gboolean
update_drive (GVfsStoragedDrive *drive)
{
  StoragedClient *storaged_client;
  gboolean changed;
  GIcon *old_icon;
  GIcon *old_symbolic_icon;
  gchar *old_name;
  gchar *old_sort_key;
  gchar *old_device_file;
  dev_t old_dev;
  gboolean old_is_media_removable;
  gboolean old_has_media;
  gboolean old_can_eject;
  gboolean old_can_stop;
  StoragedBlock *block;
#if STORAGED_CHECK_VERSION(2,0,90)
  StoragedObjectInfo *info = NULL;
#endif

  storaged_client = gvfs_storaged_volume_monitor_get_storaged_client (drive->monitor);

  /* ---------------------------------------------------------------------------------------------------- */
  /* save old values */

  old_is_media_removable = drive->is_media_removable;
  old_has_media = drive->has_media;
  old_can_eject = drive->can_eject;
  old_can_stop = drive->can_stop;

  old_name = g_strdup (drive->name);
  old_sort_key = g_strdup (drive->sort_key);
  old_device_file = g_strdup (drive->device_file);
  old_dev = drive->dev;
  old_icon = drive->icon != NULL ? g_object_ref (drive->icon) : NULL;
  old_symbolic_icon = drive->symbolic_icon != NULL ? g_object_ref (drive->symbolic_icon) : NULL;

  /* ---------------------------------------------------------------------------------------------------- */
  /* reset */

  drive->is_media_removable = drive->has_media = drive->can_eject = drive->can_stop = FALSE;
  g_free (drive->name); drive->name = NULL;
  g_free (drive->sort_key); drive->sort_key = NULL;
  g_free (drive->device_file); drive->device_file = NULL;
  drive->dev = 0;
  g_clear_object (&drive->icon);
  g_clear_object (&drive->symbolic_icon);

  /* ---------------------------------------------------------------------------------------------------- */
  /* in with the new */

  block = storaged_client_get_block_for_drive (storaged_client,
                                             drive->storaged_drive,
                                             FALSE);
  if (block != NULL)
    {
      drive->device_file = storaged_block_dup_device (block);
      drive->dev = storaged_block_get_device_number (block);

      g_object_unref (block);
    }

  drive->sort_key = g_strdup (storaged_drive_get_sort_key (drive->storaged_drive));

  drive->is_media_removable = storaged_drive_get_media_removable (drive->storaged_drive);
  if (drive->is_media_removable)
    {
      drive->has_media = storaged_drive_get_media_available (drive->storaged_drive);
    }
  else
    {
      drive->has_media = TRUE;
    }
  drive->can_eject = storaged_drive_get_ejectable (drive->storaged_drive);

#if STORAGED_CHECK_VERSION(2,0,90)
  {
    StoragedObject *object = (StoragedObject *) g_dbus_interface_get_object (G_DBUS_INTERFACE (drive->storaged_drive));
    if (object != NULL)
      {
        info = storaged_client_get_object_info (storaged_client, object);
        if (info != NULL)
          {
            drive->name = g_strdup (storaged_object_info_get_name (info));
            drive->icon = _g_object_ref0 (storaged_object_info_get_icon (info));
            drive->symbolic_icon = _g_object_ref0 (storaged_object_info_get_icon_symbolic (info));
          }
      }
  }
#else
  storaged_client_get_drive_info (storaged_client,
                                drive->storaged_drive,
                                NULL,         /* drive_name */
                                &drive->name,
                                &drive->icon,
                                NULL,         /* media_desc */
                                NULL);        /* media_icon */
#endif

#if STORAGED_CHECK_VERSION(2,0,90)
  {
    /* If can_stop is TRUE, then
     *
     *  - the GUI (e.g. Files, Shell) will call GDrive.stop() whenever the
     *    user presses the Eject icon, which will result in:
     *
     *  - us calling StoragedDrive.PowerOff() on GDrive.stop(), which
     *    will result in:
     *
     *  - Storaged asking the kernel to power off the USB port the drive
     *    is connected to, which will result in
     *
     *  - Most drives powering off (especially true for bus-powered
     *    drives such as 2.5" HDDs and USB sticks), which will result in
     *
     *  - Users feeling warm and cozy when they see the LED on the
     *    device turn off (win)
     *
     * Obviously this is unwanted if
     *
     *  - the drive is using removable media (e.g. optical discs,
     *    flash media etc); or
     *
     *  - the device is internal
     *
     * So for the latter, only do this for drives we appear *during*
     * the login session.  Note that this heuristic has the nice
     * side-effect that USB-attached hard disks that are plugged in
     * when the computer starts up will not be powered off when the
     * user clicks the "eject" icon.
     */
    if (!drive->is_media_removable && !drive->coldplug)
      {
        if (storaged_drive_get_can_power_off (drive->storaged_drive))
          {
            drive->can_stop = TRUE;
          }
      }
  }
#endif

  /* ---------------------------------------------------------------------------------------------------- */
  /* fallbacks */

  /* Never use empty/blank names (#582772) */
  if (drive->name == NULL || strlen (drive->name) == 0)
    {
      if (drive->device_file != NULL)
        drive->name = g_strdup_printf (_("Unnamed Drive (%s)"), drive->device_file);
      else
        drive->name = g_strdup (_("Unnamed Drive"));
    }
  if (drive->icon == NULL)
    drive->icon = g_themed_icon_new ("drive-removable-media");
  if (drive->symbolic_icon == NULL)
    drive->symbolic_icon = g_themed_icon_new ("drive-removable-media-symbolic");

  /* ---------------------------------------------------------------------------------------------------- */
  /* compute whether something changed */
  changed = !((old_is_media_removable == drive->is_media_removable) &&
              (old_has_media == drive->has_media) &&
              (old_can_eject == drive->can_eject) &&
              (old_can_stop == drive->can_stop) &&
              (g_strcmp0 (old_name, drive->name) == 0) &&
              (g_strcmp0 (old_sort_key, drive->sort_key) == 0) &&
              (g_strcmp0 (old_device_file, drive->device_file) == 0) &&
              (old_dev == drive->dev) &&
              g_icon_equal (old_icon, drive->icon) &&
              g_icon_equal (old_symbolic_icon, drive->symbolic_icon)
              );

  /* free old values */
  g_free (old_name);
  g_free (old_sort_key);
  g_free (old_device_file);
  g_clear_object (&old_icon);
  g_clear_object (&old_symbolic_icon);

  /*g_debug ("in update_drive(); has_media=%d changed=%d", drive->has_media, changed);*/

#if STORAGED_CHECK_VERSION(2,0,90)
  g_clear_object (&info);
#endif

  return changed;
}

static void
on_storaged_drive_notify (GObject     *object,
                        GParamSpec  *pspec,
                        gpointer     user_data)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (user_data);
  if (update_drive (drive))
    emit_changed (drive);
}

GVfsStoragedDrive *
gvfs_storaged_drive_new (GVfsStoragedVolumeMonitor  *monitor,
                        StoragedDrive               *storaged_drive,
                        gboolean                   coldplug)
{
  GVfsStoragedDrive *drive;

  drive = g_object_new (GVFS_TYPE_STORAGED_DRIVE, NULL);
  drive->monitor = monitor;
  drive->coldplug = coldplug;

  drive->storaged_drive = g_object_ref (storaged_drive);
  g_signal_connect (drive->storaged_drive,
                    "notify",
                    G_CALLBACK (on_storaged_drive_notify),
                    drive);

  update_drive (drive);

  return drive;
}

void
gvfs_storaged_drive_disconnected (GVfsStoragedDrive *drive)
{
  GList *l, *volumes;

  volumes = drive->volumes;
  drive->volumes = NULL;
  for (l = volumes; l != NULL; l = l->next)
    {
      GVfsStoragedVolume *volume = l->data;
      gvfs_storaged_volume_unset_drive (volume, drive);
    }
  g_list_free (volumes);
}

void
gvfs_storaged_drive_set_volume (GVfsStoragedDrive  *drive,
                               GVfsStoragedVolume *volume)
{
  if (g_list_find (drive->volumes, volume) == NULL)
    {
      drive->volumes = g_list_prepend (drive->volumes, volume);
      emit_changed (drive);
    }
}

void
gvfs_storaged_drive_unset_volume (GVfsStoragedDrive  *drive,
                                 GVfsStoragedVolume *volume)
{
  GList *l;
  l = g_list_find (drive->volumes, volume);
  if (l != NULL)
    {
      drive->volumes = g_list_delete_link (drive->volumes, l);
      emit_changed (drive);
    }
}

static GIcon *
gvfs_storaged_drive_get_icon (GDrive *_drive)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  return drive->icon != NULL ? g_object_ref (drive->icon) : NULL;
}

static GIcon *
gvfs_storaged_drive_get_symbolic_icon (GDrive *_drive)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  return drive->symbolic_icon != NULL ? g_object_ref (drive->symbolic_icon) : NULL;
}

static char *
gvfs_storaged_drive_get_name (GDrive *_drive)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  return g_strdup (drive->name);
}

static GList *
gvfs_storaged_drive_get_volumes (GDrive *_drive)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  GList *l;
  l = g_list_copy (drive->volumes);
  g_list_foreach (l, (GFunc) g_object_ref, NULL);
  return l;
}

static gboolean
gvfs_storaged_drive_has_volumes (GDrive *_drive)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  gboolean res;
  res = drive->volumes != NULL;
  return res;
}

static gboolean
gvfs_storaged_drive_is_media_removable (GDrive *_drive)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  return drive->is_media_removable;
}

static gboolean
gvfs_storaged_drive_has_media (GDrive *_drive)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  return drive->has_media;
}

static gboolean
gvfs_storaged_drive_is_media_check_automatic (GDrive *_drive)
{
  return TRUE;
}

static gboolean
gvfs_storaged_drive_can_eject (GDrive *_drive)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  return drive->can_eject;
}

static gboolean
gvfs_storaged_drive_can_poll_for_media (GDrive *_drive)
{
  return FALSE;
}

static gboolean
gvfs_storaged_drive_can_start (GDrive *_drive)
{
  return FALSE;
}

static gboolean
gvfs_storaged_drive_can_start_degraded (GDrive *_drive)
{
  return FALSE;
}

static gboolean
gvfs_storaged_drive_can_stop (GDrive *_drive)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  return drive->can_stop;
}

static GDriveStartStopType
gvfs_storaged_drive_get_start_stop_type (GDrive *_drive)
{
  return G_DRIVE_START_STOP_TYPE_SHUTDOWN;
}

/* ---------------------------------------------------------------------------------------------------- */

static char *
gvfs_storaged_drive_get_identifier (GDrive      *_drive,
                                   const gchar *kind)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  gchar *ret = NULL;

  if (drive->device_file != NULL)
    {
      if (g_strcmp0 (kind, G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE) == 0)
        ret = g_strdup (drive->device_file);
    }
  return ret;
}

static gchar **
gvfs_storaged_drive_enumerate_identifiers (GDrive *_drive)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  GPtrArray *p;

  p = g_ptr_array_new ();
  if (drive->device_file != NULL)
    g_ptr_array_add (p, g_strdup (G_VOLUME_IDENTIFIER_KIND_UNIX_DEVICE));
  g_ptr_array_add (p, NULL);

  return (gchar **) g_ptr_array_free (p, FALSE);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef void (*UnmountsMountsFunc)  (GDrive              *drive,
                                     GMountOperation     *mount_operation,
                                     GCancellable        *cancellable,
                                     GAsyncReadyCallback  callback,
                                     gpointer             user_data,
                                     gpointer             on_all_unmounted_data);

typedef struct {
  GDrive *drive;
  GAsyncReadyCallback callback;
  gpointer user_data;
  GMountOperation *mount_operation;
  GCancellable *cancellable;
  GMountUnmountFlags flags;

  GList *pending_mounts;

  UnmountsMountsFunc on_all_unmounted;
  gpointer on_all_unmounted_data;
} UnmountMountsOp;

static void
free_unmount_mounts_op (UnmountMountsOp *data)
{
  g_list_free_full (data->pending_mounts, g_object_unref);

  g_object_unref (data->drive);
  g_free (data);
}

static void
unmount_mounts_cb (GObject       *source_object,
                   GAsyncResult  *res,
                   gpointer       user_data);

static void
unmount_mounts_do (UnmountMountsOp *data)
{
  if (data->pending_mounts == NULL)
    {
      data->on_all_unmounted (data->drive,
                              data->mount_operation,
                              data->cancellable,
                              data->callback,
                              data->user_data,
                              data->on_all_unmounted_data);

      free_unmount_mounts_op (data);
    }
  else
    {
      GMount *mount;
      mount = data->pending_mounts->data;
      data->pending_mounts = g_list_remove (data->pending_mounts, mount);

      g_mount_unmount_with_operation (mount,
                                      data->flags,
                                      data->mount_operation,
                                      data->cancellable,
                                      unmount_mounts_cb,
                                      data);
    }
}

static void
unmount_mounts_cb (GObject      *source_object,
                   GAsyncResult *res,
                   gpointer      user_data)
{
  UnmountMountsOp *data = user_data;
  GMount *mount = G_MOUNT (source_object);
  GSimpleAsyncResult *simple;
  GError *error = NULL;

  if (!g_mount_unmount_with_operation_finish (mount, res, &error))
    {
      /* make the error dialog more targeted to the drive.. unless the user has already seen a dialog */
      if (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_BUSY)
        {
          g_error_free (error);
          error = g_error_new (G_IO_ERROR,
                               G_IO_ERROR_BUSY,
                               _("Failed to eject medium; one or more volumes on the medium are busy."));
        }

      if (data->mount_operation != NULL)
        gvfs_storaged_unmount_notify_stop (data->mount_operation);

      /* unmount failed; need to fail the whole eject operation */
      simple = g_simple_async_result_new_from_error (G_OBJECT (data->drive),
                                                     data->callback,
                                                     data->user_data,
                                                     error);
      g_error_free (error);
      g_simple_async_result_complete (simple);
      g_object_unref (simple);

      free_unmount_mounts_op (data);
    }
  else
    {
      /* move on to the next mount.. */
      unmount_mounts_do (data);
    }
  g_object_unref (mount);
}

static void
unmount_mounts (GVfsStoragedDrive    *drive,
                GMountUnmountFlags   flags,
                GMountOperation     *mount_operation,
                GCancellable        *cancellable,
                GAsyncReadyCallback  callback,
                gpointer             user_data,
                UnmountsMountsFunc   on_all_unmounted,
                gpointer             on_all_unmounted_data)
{
  GMount *mount;
  UnmountMountsOp *data;
  GList *l;

  data = g_new0 (UnmountMountsOp, 1);
  data->drive = g_object_ref (drive);
  data->mount_operation = mount_operation;
  data->cancellable = cancellable;
  data->callback = callback;
  data->user_data = user_data;
  data->flags = flags;
  data->on_all_unmounted = on_all_unmounted;
  data->on_all_unmounted_data = on_all_unmounted_data;

  for (l = drive->volumes; l != NULL; l = l->next)
    {
      GVfsStoragedVolume *volume = GVFS_STORAGED_VOLUME (l->data);
      mount = g_volume_get_mount (G_VOLUME (volume));
      if (mount != NULL && g_mount_can_unmount (mount))
        data->pending_mounts = g_list_prepend (data->pending_mounts, g_object_ref (mount));
    }

  unmount_mounts_do (data);
}

/* ---------------------------------------------------------------------------------------------------- */

typedef struct
{
  GSimpleAsyncResult *simple;

  GVfsStoragedDrive *drive;
  GMountOperation *mount_operation;
} EjectData;

static void
eject_data_free (EjectData *data)
{
  g_object_unref (data->simple);
  g_clear_object (&data->drive);
  g_clear_object (&data->mount_operation);

  g_free (data);
}

static void
eject_cb (GObject      *source_object,
          GAsyncResult *res,
          gpointer      user_data)
{
  EjectData *data = user_data;
  GError *error;

  error = NULL;
  if (!storaged_drive_call_eject_finish (STORAGED_DRIVE (source_object), res, &error))
    {
      gvfs_storaged_utils_storaged_error_to_gio_error (error);
      g_simple_async_result_take_error (data->simple, error);
    }

  if (data->mount_operation != NULL)
    {
      /* If we fail send an ::aborted signal to make any notification go away */
      if (error != NULL)
        g_signal_emit_by_name (data->mount_operation, "aborted");

      gvfs_storaged_unmount_notify_stop (data->mount_operation);
    }

  g_simple_async_result_complete (data->simple);
  eject_data_free (data);
}

static void
gvfs_storaged_drive_eject_on_all_unmounted (GDrive              *_drive,
                                           GMountOperation     *mount_operation,
                                           GCancellable        *cancellable,
                                           GAsyncReadyCallback  callback,
                                           gpointer             user_data,
                                           gpointer             on_all_unmounted_data)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  GVariantBuilder builder;
  EjectData *data;

  data = g_new0 (EjectData, 1);
  data->simple = g_simple_async_result_new (G_OBJECT (drive),
                                            callback,
                                            user_data,
                                            gvfs_storaged_drive_eject_on_all_unmounted);
  data->drive = g_object_ref (drive);
  if (mount_operation != NULL)
    data->mount_operation = g_object_ref (mount_operation);

  g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
  if (mount_operation == NULL)
    {
      g_variant_builder_add (&builder,
                             "{sv}",
                             "auth.no_user_interaction", g_variant_new_boolean (TRUE));
    }
  storaged_drive_call_eject (drive->storaged_drive,
                           g_variant_builder_end (&builder),
                           cancellable,
                           eject_cb,
                           data);
}

static void
gvfs_storaged_drive_eject_with_operation (GDrive              *_drive,
                                         GMountUnmountFlags   flags,
                                         GMountOperation     *mount_operation,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);

  /* This information is needed in GVfsDdisks2Volume when apps have
   * open files on the device ... we need to know if the button should
   * be "Unmount Anyway", "Eject Anyway" or "Power Off Anyway"
   */
  if (mount_operation != NULL)
    {
      g_object_set_data (G_OBJECT (mount_operation), "x-storaged-is-eject", GINT_TO_POINTER (1));
      gvfs_storaged_unmount_notify_start (mount_operation, NULL, _drive, FALSE);
    }

  /* first we need to go through all the volumes and unmount their assoicated mounts (if any) */
  unmount_mounts (drive,
                  flags,
                  mount_operation,
                  cancellable,
                  callback,
                  user_data,
                  gvfs_storaged_drive_eject_on_all_unmounted,
                  NULL);
}

static gboolean
gvfs_storaged_drive_eject_with_operation_finish (GDrive        *drive,
                                                GAsyncResult  *result,
                                                GError       **error)
{
  return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error);
}

static void
gvfs_storaged_drive_eject (GDrive              *drive,
                          GMountUnmountFlags   flags,
                          GCancellable        *cancellable,
                          GAsyncReadyCallback  callback,
                          gpointer             user_data)
{
  gvfs_storaged_drive_eject_with_operation (drive, flags, NULL, cancellable, callback, user_data);
}

static gboolean
gvfs_storaged_drive_eject_finish (GDrive        *drive,
                                 GAsyncResult  *result,
                                 GError       **error)
{
  return gvfs_storaged_drive_eject_with_operation_finish (drive, result, error);
}

/* ---------------------------------------------------------------------------------------------------- */

#if STORAGED_CHECK_VERSION(2,0,90)

typedef struct
{
  GSimpleAsyncResult *simple;

  GVfsStoragedDrive *drive;
  GMountOperation *mount_operation;
} StopData;

static void
stop_data_free (StopData *data)
{
  g_object_unref (data->simple);
  g_clear_object (&data->drive);
  g_clear_object (&data->mount_operation);

  g_free (data);
}

static void
power_off_cb (GObject      *source_object,
              GAsyncResult *res,
              gpointer      user_data)
{
  StopData *data = user_data;
  GError *error;

  error = NULL;
  if (!storaged_drive_call_power_off_finish (STORAGED_DRIVE (source_object), res, &error))
    {
      gvfs_storaged_utils_storaged_error_to_gio_error (error);
      g_simple_async_result_take_error (data->simple, error);
    }

  if (data->mount_operation != NULL)
    {
      /* If we fail send an ::aborted signal to make any notification go away */
      if (error != NULL)
        g_signal_emit_by_name (data->mount_operation, "aborted");

      gvfs_storaged_unmount_notify_stop (data->mount_operation);
    }

  g_simple_async_result_complete (data->simple);
  stop_data_free (data);
}

static void
gvfs_storaged_drive_stop_on_all_unmounted (GDrive              *_drive,
                                          GMountOperation     *mount_operation,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback  callback,
                                          gpointer             user_data,
                                          gpointer             on_all_unmounted_data)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  GVariantBuilder builder;
  StopData *data;

  data = g_new0 (StopData, 1);
  data->simple = g_simple_async_result_new (G_OBJECT (drive),
                                            callback,
                                            user_data,
                                            gvfs_storaged_drive_stop_on_all_unmounted);
  data->drive = g_object_ref (drive);
  if (mount_operation != NULL)
    data->mount_operation = g_object_ref (mount_operation);

  g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
  if (mount_operation == NULL)
    {
      g_variant_builder_add (&builder,
                             "{sv}",
                             "auth.no_user_interaction", g_variant_new_boolean (TRUE));
    }
  storaged_drive_call_power_off (drive->storaged_drive,
                               g_variant_builder_end (&builder),
                               cancellable,
                               power_off_cb,
                               data);
}

static void
gvfs_storaged_drive_stop (GDrive              *_drive,
                         GMountUnmountFlags   flags,
                         GMountOperation     *mount_operation,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);

  /* This information is needed in GVfsDdisks2Volume when apps have
   * open files on the device ... we need to know if the button should
   * be "Unmount Anyway", "Eject Anyway" or "Power Off Anyway"
   */
  if (mount_operation != NULL)
    {
      g_object_set_data (G_OBJECT (mount_operation), "x-storaged-is-stop", GINT_TO_POINTER (1));
      gvfs_storaged_unmount_notify_start (mount_operation, NULL, _drive, FALSE);
    }

  /* first we need to go through all the volumes and unmount their assoicated mounts (if any) */
  unmount_mounts (drive,
                  flags,
                  mount_operation,
                  cancellable,
                  callback,
                  user_data,
                  gvfs_storaged_drive_stop_on_all_unmounted,
                  NULL);
}

static gboolean
gvfs_storaged_drive_stop_finish (GDrive        *drive,
                                GAsyncResult  *result,
                                GError       **error)
{
  return !g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), error);
}

#endif /* STORAGED_CHECK_VERSION(2,0,90) */

/* ---------------------------------------------------------------------------------------------------- */

static const gchar *
gvfs_storaged_drive_get_sort_key (GDrive *_drive)
{
  GVfsStoragedDrive *drive = GVFS_STORAGED_DRIVE (_drive);
  return drive->sort_key;
}

/* ---------------------------------------------------------------------------------------------------- */

static void
gvfs_storaged_drive_drive_iface_init (GDriveIface *iface)
{
  iface->get_name = gvfs_storaged_drive_get_name;
  iface->get_icon = gvfs_storaged_drive_get_icon;
  iface->get_symbolic_icon = gvfs_storaged_drive_get_symbolic_icon;
  iface->has_volumes = gvfs_storaged_drive_has_volumes;
  iface->get_volumes = gvfs_storaged_drive_get_volumes;
  iface->is_media_removable = gvfs_storaged_drive_is_media_removable;
  iface->has_media = gvfs_storaged_drive_has_media;
  iface->is_media_check_automatic = gvfs_storaged_drive_is_media_check_automatic;
  iface->can_eject = gvfs_storaged_drive_can_eject;
  iface->can_poll_for_media = gvfs_storaged_drive_can_poll_for_media;
  iface->get_identifier = gvfs_storaged_drive_get_identifier;
  iface->enumerate_identifiers = gvfs_storaged_drive_enumerate_identifiers;
  iface->get_start_stop_type = gvfs_storaged_drive_get_start_stop_type;
  iface->can_start = gvfs_storaged_drive_can_start;
  iface->can_start_degraded = gvfs_storaged_drive_can_start_degraded;
  iface->can_stop = gvfs_storaged_drive_can_stop;
  iface->eject = gvfs_storaged_drive_eject;
  iface->eject_finish = gvfs_storaged_drive_eject_finish;
  iface->eject_with_operation = gvfs_storaged_drive_eject_with_operation;
  iface->eject_with_operation_finish = gvfs_storaged_drive_eject_with_operation_finish;
  iface->get_sort_key = gvfs_storaged_drive_get_sort_key;
#if 0
  iface->poll_for_media = gvfs_storaged_drive_poll_for_media;
  iface->poll_for_media_finish = gvfs_storaged_drive_poll_for_media_finish;
  iface->start = gvfs_storaged_drive_start;
  iface->start_finish = gvfs_storaged_drive_start_finish;
#endif

#if STORAGED_CHECK_VERSION(2,0,90)
  iface->stop = gvfs_storaged_drive_stop;
  iface->stop_finish = gvfs_storaged_drive_stop_finish;
#endif
}

StoragedDrive *
gvfs_storaged_drive_get_storaged_drive (GVfsStoragedDrive *drive)
{
  return drive->storaged_drive;
}
