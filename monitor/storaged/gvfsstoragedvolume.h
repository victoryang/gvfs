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

#ifndef __GVFS_STORAGED_VOLUME_H__
#define __GVFS_STORAGED_VOLUME_H__

#include <glib-object.h>
#include <gio/gio.h>

#include "gvfsstoragedvolumemonitor.h"

G_BEGIN_DECLS

#define GVFS_TYPE_STORAGED_VOLUME  (gvfs_storaged_volume_get_type ())
#define GVFS_STORAGED_VOLUME(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GVFS_TYPE_STORAGED_VOLUME, GVfsStoragedVolume))
#define GVFS_IS_STORAGED_VOLUME(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GVFS_TYPE_STORAGED_VOLUME))

GType              gvfs_storaged_volume_get_type    (void) G_GNUC_CONST;

GVfsStoragedVolume *gvfs_storaged_volume_new         (GVfsStoragedVolumeMonitor   *monitor,
                                                    StoragedBlock                *block,
                                                    GUnixMountPoint            *mount_point,
                                                    GVfsStoragedDrive           *drive,
                                                    GFile                      *activation_root,
                                                    gboolean                    coldplug);
void               gvfs_storaged_volume_removed     (GVfsStoragedVolume          *volume);

StoragedBlock       *gvfs_storaged_volume_get_block       (GVfsStoragedVolume      *volume);
GUnixMountPoint   *gvfs_storaged_volume_get_mount_point (GVfsStoragedVolume      *volume);
dev_t              gvfs_storaged_volume_get_dev         (GVfsStoragedVolume      *volume);

void               gvfs_storaged_volume_set_mount   (GVfsStoragedVolume          *volume,
                                                    GVfsStoragedMount           *mount);
void               gvfs_storaged_volume_unset_mount (GVfsStoragedVolume          *volume,
                                                    GVfsStoragedMount           *mount);

void               gvfs_storaged_volume_set_drive   (GVfsStoragedVolume          *volume,
                                                    GVfsStoragedDrive           *drive);
void               gvfs_storaged_volume_unset_drive (GVfsStoragedVolume          *volume,
                                                    GVfsStoragedDrive           *drive);

gboolean           gvfs_storaged_volume_has_uuid    (GVfsStoragedVolume          *volume,
                                                    const gchar                *uuid);

G_END_DECLS

#endif /* __GVFS_STORAGED_VOLUME_H__ */
