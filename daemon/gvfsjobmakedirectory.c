/* GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include <config.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <glib.h>
#include <glib/gi18n.h>
#include "gvfsjobmakedirectory.h"
#include "gvfsdaemonprotocol.h"

G_DEFINE_TYPE (GVfsJobMakeDirectory, g_vfs_job_make_directory, G_VFS_TYPE_JOB_DBUS)

static void         run          (GVfsJob        *job);
static gboolean     try          (GVfsJob        *job);
static void         create_reply (GVfsJob               *job,
                                  GVfsDBusMount         *object,
                                  GDBusMethodInvocation *invocation);

static void
g_vfs_job_make_directory_finalize (GObject *object)
{
  GVfsJobMakeDirectory *job;

  job = G_VFS_JOB_MAKE_DIRECTORY (object);
  
  g_free (job->filename);
  
  if (G_OBJECT_CLASS (g_vfs_job_make_directory_parent_class)->finalize)
    (*G_OBJECT_CLASS (g_vfs_job_make_directory_parent_class)->finalize) (object);
}

static void
g_vfs_job_make_directory_class_init (GVfsJobMakeDirectoryClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GVfsJobClass *job_class = G_VFS_JOB_CLASS (klass);
  GVfsJobDBusClass *job_dbus_class = G_VFS_JOB_DBUS_CLASS (klass);
  
  gobject_class->finalize = g_vfs_job_make_directory_finalize;
  job_class->run = run;
  job_class->try = try;
  job_dbus_class->create_reply = create_reply;
}

static void
g_vfs_job_make_directory_init (GVfsJobMakeDirectory *job)
{
}

gboolean
g_vfs_job_make_directory_new_handle (GVfsDBusMount *object,
                                     GDBusMethodInvocation *invocation,
                                     const gchar *arg_path_data,
                                     GVfsBackend *backend)
{
  GVfsJobMakeDirectory *job;

  if (g_vfs_backend_invocation_first_handler (object, invocation, backend))
    return TRUE;
  
  job = g_object_new (G_VFS_TYPE_JOB_MAKE_DIRECTORY,
                      "object", object,
                      "invocation", invocation,
                      NULL);

  job->filename = g_strdup (arg_path_data);
  job->backend = backend;
  
  g_vfs_job_source_new_job (G_VFS_JOB_SOURCE (backend), G_VFS_JOB (job));
  g_object_unref (job);

  return TRUE;
}

static void
run (GVfsJob *job)
{
  GVfsJobMakeDirectory *op_job = G_VFS_JOB_MAKE_DIRECTORY (job);
  GVfsBackendClass *class = G_VFS_BACKEND_GET_CLASS (op_job->backend);

  if (class->make_directory == NULL)
    {
      g_vfs_job_failed (job, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
			_("Operation not supported by backend"));
      return;
    }
  
  class->make_directory (op_job->backend,
			 op_job,
			 op_job->filename);
}

static gboolean
try (GVfsJob *job)
{
  GVfsJobMakeDirectory *op_job = G_VFS_JOB_MAKE_DIRECTORY (job);
  GVfsBackendClass *class = G_VFS_BACKEND_GET_CLASS (op_job->backend);

  if (class->try_make_directory == NULL)
    return FALSE;
  
  return class->try_make_directory (op_job->backend,
				    op_job,
				    op_job->filename);
}

/* Might be called on an i/o thread */
static void
create_reply (GVfsJob *job,
              GVfsDBusMount *object,
              GDBusMethodInvocation *invocation)
{
  gvfs_dbus_mount_complete_make_directory (object, invocation);
}
