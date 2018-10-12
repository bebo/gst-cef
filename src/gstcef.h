/* GStreamer
 * Copyright (C) 2017 FIXME <fixme@example.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_CEF_H_
#define _GST_CEF_H_

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <gst/video/gstvideometa.h>
#include <gst/video/video.h>

G_BEGIN_DECLS

#define GST_TYPE_CEF (gst_cef_get_type())
#define GST_CEF(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_CEF, GstCef))
#define GST_CEF_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_CEF, GstCefClass))
#define GST_IS_CEF(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_CEF))
#define GST_IS_CEF_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_CEF))

typedef struct _GstCef GstCef;
typedef struct _GstCefClass GstCefClass;

struct _GstCef
{
  GstPushSrc src;
  const char *url;
  const char *js;
  const char *initialization_js;
  const char *local_filepath;
  const char *cache_path;

  // Need the frame_mutex to touch the below properties.
  GMutex frame_mutex;
  GstBuffer *current_buffer;
  GCond frame_cond;
  GCond buffer_cond;
  /* gint64 cur_offset; */
  volatile gint unlocked;
  volatile gint has_new_frame;
  volatile gboolean has_opened_browser;
  int width;
  int height;
  gboolean hidden;
};

struct _GstCefClass
{
  GstPushSrcClass base_cef_class;
};

GType gst_cef_get_type(void);

G_END_DECLS

#endif
