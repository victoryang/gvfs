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

#ifndef __GVFS_STORAGED_MOUNT_H__
#define __GVFS_STORAGED_MOUNT_H__

#include <glib-object.h>
#include <gio/gio.h>

#include "gvfsstoragedvolumemonitor.h"

G_BEGIN_DECLS

#define GVFS_TYPE_STORAGED_MOUNT  (gvfs_storaged_mount_get_type ())
#define GVFS_STORAGED_MOUNT(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GVFS_TYPE_STORAGED_MOUNT, GVfsStoragedMount))
#define GVFS_IS_STORAGED_MOUNT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GVFS_TYPE_STORAGED_MOUNT))


GType             gvfs_storaged_mount_get_type       (void) G_GNUC_CONST;
GVfsStoragedMount *gvfs_storaged_mount_new            (GVfsStoragedVolumeMonitor *monitor,
                                                     GUnixMountEntry          *mount_entry,
                                                     GVfsStoragedVolume        *volume);
void              gvfs_storaged_mount_unmounted      (GVfsStoragedMount         *mount);

gboolean          gvfs_storaged_mount_has_uuid       (GVfsStoragedMount         *mount,
                                                     const gchar              *uuid);

void              gvfs_storaged_mount_set_volume     (GVfsStoragedMount         *mount,
                                                     GVfsStoragedVolume        *volume);
void              gvfs_storaged_mount_unset_volume   (GVfsStoragedMount         *mount,
                                                     GVfsStoragedVolume        *volume);
gboolean          gvfs_storaged_mount_has_volume     (GVfsStoragedMount         *mount,
                                                     GVfsStoragedVolume        *volume);
GVfsStoragedVolume *gvfs_storaged_mount_get_volume    (GVfsStoragedMount         *mount);

const gchar      *gvfs_storaged_mount_get_mount_path  (GVfsStoragedMount        *mount);
GUnixMountEntry  *gvfs_storaged_mount_get_mount_entry (GVfsStoragedMount        *mount);

G_END_DECLS

#endif /* __GVFS_STORAGED_MOUNT_H__ */
