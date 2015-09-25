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

#ifndef __GVFS_STORAGED_DRIVE_H__
#define __GVFS_STORAGED_DRIVE_H__

#include <glib-object.h>
#include <gio/gio.h>

#include "gvfsstoragedvolumemonitor.h"

G_BEGIN_DECLS

#define GVFS_TYPE_STORAGED_DRIVE  (gvfs_storaged_drive_get_type ())
#define GVFS_STORAGED_DRIVE(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GVFS_TYPE_STORAGED_DRIVE, GVfsStoragedDrive))
#define GVFS_IS_STORAGED_DRIVE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GVFS_TYPE_STORAGED_DRIVE))

GType             gvfs_storaged_drive_get_type        (void) G_GNUC_CONST;
GVfsStoragedDrive *gvfs_storaged_drive_new             (GVfsStoragedVolumeMonitor *monitor,
                                                      StoragedDrive              *storaged_drive,
                                                      gboolean                   coldplug);
void              gvfs_storaged_drive_disconnected    (GVfsStoragedDrive         *drive);

void              gvfs_storaged_drive_set_volume      (GVfsStoragedDrive         *drive,
                                                      GVfsStoragedVolume        *volume);
void              gvfs_storaged_drive_unset_volume    (GVfsStoragedDrive         *drive,
                                                      GVfsStoragedVolume        *volume);
StoragedDrive     *gvfs_storaged_drive_get_storaged_drive (GVfsStoragedDrive         *drive);

G_END_DECLS

#endif /* __GVFS_STORAGED_DRIVE_H__ */
