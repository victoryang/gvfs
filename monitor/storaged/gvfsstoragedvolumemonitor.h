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

#ifndef __GVFS_STORAGED_VOLUME_MONITOR_H__
#define __GVFS_STORAGED_VOLUME_MONITOR_H__

#include <glib-object.h>
#include <gio/gio.h>
#include <gio/gunixmounts.h>

#include <storaged/storaged.h>
#include <gudev/gudev.h>

G_BEGIN_DECLS

#define GVFS_TYPE_STORAGED_VOLUME_MONITOR  (gvfs_storaged_volume_monitor_get_type ())
#define GVFS_STORAGED_VOLUME_MONITOR(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GVFS_TYPE_STORAGED_VOLUME_MONITOR, GVfsStoragedVolumeMonitor))
#define GVFS_IS_STORAGED_VOLUME_MONITOR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GVFS_TYPE_STORAGED_VOLUME_MONITOR))

typedef struct _GVfsStoragedVolumeMonitor GVfsStoragedVolumeMonitor;

/* Forward definitions */
typedef struct _GVfsStoragedDrive GVfsStoragedDrive;
typedef struct _GVfsStoragedVolume GVfsStoragedVolume;
typedef struct _GVfsStoragedMount GVfsStoragedMount;

GType           gvfs_storaged_volume_monitor_get_type          (void) G_GNUC_CONST;
GVolumeMonitor *gvfs_storaged_volume_monitor_new               (void);
StoragedClient   *gvfs_storaged_volume_monitor_get_storaged_client (GVfsStoragedVolumeMonitor *monitor);
void            gvfs_storaged_volume_monitor_update            (GVfsStoragedVolumeMonitor *monitor);
GUdevClient    *gvfs_storaged_volume_monitor_get_gudev_client  (GVfsStoragedVolumeMonitor *monitor);

G_END_DECLS

#endif /* __GVFS_STORAGED_VOLUME_MONITOR_H__ */
