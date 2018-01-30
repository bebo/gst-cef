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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstcef
 *
 * The cef element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! cef ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */
/* #define _GNU_SOURCE     /1* To get pthread_getattr_np() declaration *1/ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include "gstcef.h"
#include "cef.h"

GST_DEBUG_CATEGORY(gst_cef_debug_category);
#define DEFAULT_IS_LIVE            TRUE

static void gst_cef_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_cef_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_cef_dispose (GObject * object);
static void gst_cef_finalize (GObject * object);

static GstCaps *gst_cef_get_caps (GstBaseSrc * src, GstCaps * filter);
static gboolean gst_cef_is_seekable (GstBaseSrc * src);
static gboolean gst_cef_unlock (GstBaseSrc * src);
static gboolean gst_cef_unlock_stop (GstBaseSrc * src);
static GstFlowReturn gst_cef_create (GstBaseSrc * src, GstBuffer ** buf);
static GstStateChangeReturn gst_cef_change_state (GstElement * element, GstStateChange transition);
static gboolean gst_cef_start (GstBaseSrc *src);
static gboolean gst_cef_stop (GstBaseSrc *src);

enum
{
  PROP_0,
  PROP_URL,
  PROP_WIDTH,
  PROP_HEIGHT
};

/* pad templates */
#define VTS_VIDEO_CAPS GST_VIDEO_CAPS_MAKE ("BGRA")

static GstStaticPadTemplate gst_cef_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (VTS_VIDEO_CAPS)
    );


/* class initialization */

static GstElementClass *parent_element_class = NULL;

G_DEFINE_TYPE_WITH_CODE (GstCef, gst_cef, GST_TYPE_PUSH_SRC,
  GST_DEBUG_CATEGORY_INIT (gst_cef_debug_category, "cef", 0,
  "debug category for cef element"));

static GThread *browserLoop;

static void
gst_cef_class_init (GstCefClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  printf("gst_cef_class_init\n");
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS (klass);
  GstPushSrcClass *push_src_class = GST_PUSH_SRC_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
  parent_element_class = element_class;

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS(klass),
      &gst_cef_src_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "Gstreamer chromium embedded (cef)", "Generic", "FIXME Description",
      "Florian P. Nierhaus <fpn@bebo.com>");

  gobject_class->set_property = gst_cef_set_property;
  gobject_class->get_property = gst_cef_get_property;

  gobject_class->dispose = gst_cef_dispose;
  base_src_class->get_caps = GST_DEBUG_FUNCPTR (gst_cef_get_caps);
  base_src_class->is_seekable = GST_DEBUG_FUNCPTR (gst_cef_is_seekable);
  base_src_class->unlock = GST_DEBUG_FUNCPTR (gst_cef_unlock);
  base_src_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_cef_unlock_stop);
  base_src_class->start = GST_DEBUG_FUNCPTR (gst_cef_start);
  base_src_class->stop = GST_DEBUG_FUNCPTR (gst_cef_stop);
  push_src_class->create = GST_DEBUG_FUNCPTR (gst_cef_create);

  g_object_class_install_property (gobject_class, PROP_URL,
      g_param_spec_string ("url", "url", "website to render into video",
        "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WIDTH,
      g_param_spec_uint ("width", "width", "website to render into video",
        0, G_MAXUINT, 1920, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HEIGHT,
      g_param_spec_uint ("height", "height", "website to render into video",
        0, G_MAXUINT, 1080, G_PARAM_READWRITE));
}

static void push_frame(void *gstCef, const void *buffer, int width, int height) {
  GstCef *cef = (GstCef *) gstCef;
  int size = width * height * 4 * 1;
  GstBuffer *buf;
  buf = gst_buffer_new_allocate (NULL, size, NULL);
  if (G_UNLIKELY (buf == NULL)) {
    GST_ERROR_OBJECT (cef, "Failed to allocate %u bytes", size);
    return;
  }

  GstMapInfo map;
  gst_buffer_map(buf, &map, GST_MAP_WRITE);
  memcpy(map.data, buffer, size);
  gst_buffer_unmap (buf, &map);

  g_mutex_lock(&cef->frame_mutex);

  if (cef->current_frame) {
    gst_buffer_unref(cef->current_frame);
  }

  cef->current_frame = buf;
  g_atomic_int_set(&cef->has_new_frame, 1);

  g_cond_signal (&cef->frame_cond);
  g_mutex_unlock (&cef->frame_mutex);
}

GstBuffer * pop_frame(GstCef *cef)
{

  g_mutex_lock (&cef->frame_mutex);

  GstBuffer * frame = NULL;
  gint64 end_time;

  while (g_atomic_int_get(&cef->has_new_frame) == 0 && g_atomic_int_get(&cef->unlocked) == 0) {
    g_cond_wait (&cef->frame_cond, &cef->frame_mutex);
  }

  if (g_atomic_int_get(&cef->unlocked) == 0) { // 0 - not in cleanup state
    if(cef->current_frame) {
      frame = cef->current_frame;

      gst_buffer_ref(cef->current_frame);
      g_atomic_int_set(&cef->has_new_frame, 0);

      g_mutex_unlock (&cef->frame_mutex);
      return frame;
    }
  } else {
    if (cef->current_frame) {
      gst_buffer_unref(cef->current_frame);
      cef->current_frame = NULL;
    }
  }
  GST_DEBUG("no frame????");
  
  g_mutex_unlock (&cef->frame_mutex);
  return NULL;
}

void new_browser(GstCef *cef) {
  const GstStructure *structure;
  int width;
  int height;
  struct gstCb *cb = g_malloc(sizeof(struct gstCb));

  GST_INFO("actual new browser");
 
  cb->gstCef = cef;
  cb->push_frame = push_frame;
  cb->url = g_strdup(cef->url);
  cb->width = cef->width;
  cb->height = cef->height;

  GST_INFO("set cb");

  if (browserLoop == 0) {
    GST_INFO("making browser loop");
    browserLoop = g_thread_ref(g_thread_new("browser_loop", (GThreadFunc)browser_loop, cb));
  } else {
    GST_INFO("open browser");
    open_browser(cb);
  }
}

void gst_cef_init(GstCef *cef)
{
  printf("gst_cef_init\n");

  g_atomic_int_set (&cef->unlocked, 0);
  cef->current_frame = NULL;
  cef->has_opened_browser = FALSE;
  cef->width=-1;
  cef->height=-1;

  gst_base_src_set_format (GST_BASE_SRC (cef), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (cef), DEFAULT_IS_LIVE);
  gst_base_src_set_do_timestamp (GST_BASE_SRC (cef), TRUE);
}

void
gst_cef_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCef *cef = GST_CEF (object);

  GST_DEBUG_OBJECT (cef, "set_property");

  switch (property_id) {
    case PROP_URL:
      {
        const gchar *url;
        url = g_value_get_string (value);
        g_free (cef->url);
        cef->url = g_strdup (url);
        break;
      }
    case PROP_WIDTH:
      {
        const width = g_value_get_uint (value);
        cef->width = width;
        break;
      }
    case PROP_HEIGHT:
      {
        const height = g_value_get_uint (value);
        cef->height = height;
        break;
      }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void gst_cef_set_size (GObject *object, int width, int height) {
  GstCef *cef = GST_CEF (object);
  struct gstSizeArgs *args = g_malloc(sizeof(struct gstSizeArgs));
  args->gstCef = cef;
  args->width = width;
  args->height = height;

  set_size(args);
  GST_INFO("setting size");
}

void
gst_cef_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstCef *cef = GST_CEF (object);

  GST_DEBUG_OBJECT (cef, "get_property");
  switch (property_id) {
    case PROP_URL:
      g_value_set_string(value, cef->url);
      break;
    case PROP_WIDTH:
      g_value_set_uint(value, cef->width);
      break;
    case PROP_HEIGHT:
      g_value_set_uint(value, cef->height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_cef_dispose (GObject * object)
{
  GstCef *cef = GST_CEF (object);

  GST_DEBUG_OBJECT (cef, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_cef_parent_class)->dispose (object);
}

void
gst_cef_finalize (GObject * object)
{
  GstCef *cef = GST_CEF (object);

  GST_DEBUG_OBJECT (cef, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_cef_parent_class)->finalize (object);
}

/* get caps from subclass */
static GstCaps *
gst_cef_get_caps (GstBaseSrc * src, GstCaps * filter)
{
  GstCef *cef = GST_CEF (src);
  GstCaps *caps;

  GST_DEBUG_OBJECT (cef, "get_caps");

  caps = gst_caps_new_simple ("video/x-raw",
      "format", G_TYPE_STRING, "BGRA",
      "framerate", GST_TYPE_FRACTION, 0, 1,
      "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
      "width", G_TYPE_INT, cef->width,
      "height", G_TYPE_INT, cef->height,
      NULL);

  return caps;
}

/* check if the resource is seekable */
static gboolean
gst_cef_is_seekable (GstBaseSrc * src)
{
  return FALSE;
}

static gboolean
gst_cef_unlock (GstBaseSrc * src)
{
  GstCef *cef = GST_CEF (src);

  GST_INFO_OBJECT (cef, "unlock");

  g_mutex_lock(&cef->frame_mutex);

  close_browser(cef);

  g_atomic_int_set (&cef->unlocked, 1);
  g_cond_signal(&cef->frame_cond);

  g_mutex_unlock(&cef->frame_mutex);

  GST_INFO_OBJECT (cef, "unlock complete");
  return TRUE;
}

static gboolean gst_cef_start (GstBaseSrc *src) {
  GstCef *cef = GST_CEF (src);
  gint width = cef->width;
  gint height = cef->width;
  char *url = cef->url;

  GST_INFO_OBJECT (cef, "start");

  if(!width || !height || !url) {
    GST_ERROR("no width, or height, or url");
    return FALSE;
  }

  new_browser(cef);

  gst_base_src_start_complete(src, GST_FLOW_OK);

  return TRUE;
}

static gboolean gst_cef_stop (GstBaseSrc *src) {
  GstCef *cef = GST_CEF (src);
  GST_INFO_OBJECT (cef, "stop");
  close_browser(cef);
  return TRUE;
}

/* Clear any pending unlock request, as we succeeded in unlocking */
static gboolean
gst_cef_unlock_stop (GstBaseSrc * src)
{
  GstCef *cef = GST_CEF (src);
  gint width = cef->width;
  gint height = cef->width;
  char *url = cef->url;

  GST_INFO_OBJECT (cef, "unlock_stop");

  g_mutex_lock(&cef->frame_mutex);

  if(!width || !height || !url) {
    GST_ERROR("no width, or height, or url");
    return FALSE;
  }

  g_atomic_int_set (&cef->unlocked, 0);

  g_mutex_unlock(&cef->frame_mutex);

  GST_INFO_OBJECT (cef, "unlock_stop complete");
  return TRUE;
}

/* notify subclasses of an event */
static gboolean
gst_cef_event (GstBaseSrc * src, GstEvent * event)
{
  GstCef *cef = GST_CEF (src);

  GST_INFO_OBJECT (cef, "event");
  GstEventType type = GST_EVENT_TYPE(event);
  switch (type) {
    default:
      GST_INFO("event_type: %s", GST_EVENT_TYPE_NAME(event));
  }

  return TRUE;
}

/* ask the subclass to create a buffer with offset and size, the default
 * implementation will call alloc and fill. */
static GstFlowReturn
gst_cef_create (GstBaseSrc * src, GstBuffer ** buf)
{
  GstCef *cef = GST_CEF (src);

  GstBuffer* buffer = pop_frame(cef);
  if (!buffer) {
    GST_INFO_OBJECT (cef, "neil create gst_flow_flushing");
    return GST_FLOW_FLUSHING;
  }

  *buf = buffer;

  return GST_FLOW_OK;
}

static GstStateChangeReturn gst_cef_change_state (GstElement * element, GstStateChange transition) {
  GstCef *cef = GST_CEF (element);
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  ret = GST_ELEMENT_CLASS (parent_element_class)->change_state (element, transition);
  return ret;
}


static gboolean
plugin_init (GstPlugin * plugin)
{

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register (plugin, "cef", GST_RANK_NONE,
      GST_TYPE_CEF);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.1"
#endif
#ifndef PACKAGE
#define PACKAGE "cef"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "gst_cef"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://github.com/bebo/gst-cef/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    cef,
    "Chromium Embedded src plugin",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)

