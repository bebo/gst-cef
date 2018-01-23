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
#include "gstcef.h"
#include "cef.h"

GST_DEBUG_CATEGORY(gst_cef_debug_category);
#define DEFAULT_IS_LIVE            TRUE

/* prototypes */

static GObjectClass *parent_class = NULL;
static GstBinClass *parent_bin_class = NULL;
static GstElementClass *parent_element_class = NULL;

static void gst_cef_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_cef_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_cef_dispose (GObject * object);
static GstStateChangeReturn gst_cef_change_state(GstElement *element, GstStateChange transition);
void cleanup(GstCef *cef);

enum
{
  PROP_0,
  PROP_URL,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_CLEANUP
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

G_DEFINE_TYPE_WITH_CODE (GstCef, gst_cef, GST_TYPE_BIN,
  GST_DEBUG_CATEGORY_INIT (gst_cef_debug_category, "cef", 0,
  "debug category for cef element"));

static GThread *browserLoop;

static void
gst_cef_class_init (GstCefClass * klass)
{
  printf("gst_cef_class_init\n");
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = gobject_class;
  GstBinClass *bin_class = GST_BIN_CLASS(klass);
  parent_bin_class = bin_class;
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
  parent_element_class = element_class;

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS(klass),
      &gst_cef_src_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "Gstreamer chromium embedded (cef)", "Generic", "FIXME Description",
      "Florian P. Nierhaus <fpn@bebo.com>");

  gobject_class->dispose = gst_cef_dispose;
  gobject_class->set_property = gst_cef_set_property;
  gobject_class->get_property = gst_cef_get_property;
  //parent_element_class->change_state = gst_cef_change_state;

  g_object_class_install_property (gobject_class, PROP_URL,
      g_param_spec_string ("url", "url", "website to render into video",
        "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_WIDTH,
      g_param_spec_uint ("width", "width", "website to render into video",
        0, 1920, 1280, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HEIGHT,
      g_param_spec_uint ("height", "height", "website to render into video",
        0, 1080, 720, G_PARAM_READWRITE));

  //TODO: this is just hacky to make it work. Want to cleanup on the change_state
  g_object_class_install_property (gobject_class, PROP_CLEANUP,
      g_param_spec_boolean ("cleanup", "cleanup", "hacky way to cleanup resources because it's late and I couldn't figure out the change_state function",
        FALSE, G_PARAM_READWRITE));
}

//TODO: this crashes and I don't know why
static GstStateChangeReturn gst_cef_change_state(GstElement *element, GstStateChange transition) {

  GstCef *cef = GST_CEF (element);
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_NULL:
      cleanup(cef);
      break;
    default:
      break;
  }

  return ret;
}

void cleanup(GstCef *cef) {
  GST_INFO("cleanup");
  if(cef->url) {
    g_free(cef->url);
    cef->url = NULL;
  }

  if(cef->appsrc) {
    gst_app_src_end_of_stream(cef->appsrc);
    gst_object_unref(cef->appsrc);
    cef->appsrc = NULL;
  }

  close_browser(cef);
}

static void gst_cef_dispose(GObject *object) {
  GstCef *cef = GST_CEF(object);
  cleanup(cef);
  parent_class->dispose(object);
}

static void push_frame(void *gstCef, const void *buffer, int width, int height) {
  GstCef *cef = (GstCef *) gstCef;
  const int size = width * height * 4 * 1;

  GST_DEBUG("push_frame: %u, %u x %u", size, width, height);
  GstBuffer *buf;
  buf = gst_buffer_new_allocate (NULL, size, NULL);
  if (G_UNLIKELY (buf == NULL)) {
    GST_ERROR_OBJECT (cef, "Failed to allocate %u bytes", size);
    return;
  }

  GstAppSrc *appsrc = cef->appsrc;

  //TODO: save them if they didn't change and use the same ones???
  GstCaps *caps = gst_caps_new_simple ("video/x-raw",
      "format", G_TYPE_STRING, "BGRA",
      "framerate", GST_TYPE_FRACTION, 0, 1,
      "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
      "width", G_TYPE_INT, width,
      "height", G_TYPE_INT, height,
      NULL);

  GstMapInfo map;
  gst_buffer_map(buf, &map, GST_MAP_WRITE);
  memcpy(map.data, buffer, size);
  gst_buffer_unmap (buf, &map);

  GstSample *sample = gst_sample_new(buf, caps, NULL, NULL);
  gst_buffer_unref(buf);
  //TODO: listen to return here
  gst_app_src_push_sample(appsrc, sample);
  gst_sample_unref(sample);
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
    browserLoop = g_thread_ref(g_thread_new("browser_loop", (GThreadFunc)browser_loop, cb));
  } else {
    open_browser(cb);
  }
}

void gst_cef_init(GstCef *cef)
{
  printf("gst_cef_init\n");
  GST_INFO("cef version neil");
  GstBin *bin = GST_BIN_CAST (cef);
  GstAppSrc *appsrc = gst_element_factory_make("appsrc", NULL);
  cef->appsrc = appsrc;
  g_object_set (G_OBJECT(appsrc), "is-live", TRUE, NULL);
  g_object_set (G_OBJECT(appsrc), "do-timestamp", TRUE, NULL);
  g_object_set (G_OBJECT(appsrc), "format", 3, NULL);
  g_object_set (G_OBJECT(appsrc), "block", TRUE, NULL);
  //TODO: make this based on width and height
  g_object_set (G_OBJECT(appsrc), "max-bytes", 2 * 1920 * 1080 * 4, NULL);

  gst_bin_add(bin, GST_ELEMENT_CAST(appsrc));

  GstPad *src_pad = gst_element_get_static_pad(GST_ELEMENT_CAST(appsrc), "src");
  gst_element_add_pad(GST_ELEMENT_CAST(bin), gst_ghost_pad_new("src", src_pad));
  gst_object_unref(GST_OBJECT(src_pad));

  cef->width=-1;
  cef->height=-1;
  cef->opened_browser = FALSE;
}

void try_new_browser(GstCef *cef) {
  char *url = cef->url;
  int width = cef->width;
  int height = cef->height;
  gboolean opened = cef->opened_browser;

  //GstState state;
  //GstElement *element = GST_ELEMENT(cef);
  //gst_element_get_state(element, &state, NULL, GST_CLOCK_TIME_NONE);

  //if(state != GST_STATE_PLAYING) {
  //  GST_DEBUG("not in playing state, not opening");
  //  return;
  //}

  if(!width) {
    GST_DEBUG("no width yet, not opening");
    return;
  }

  if(!height) {
    GST_DEBUG("no height yet, not opening");
    return;
  }

  if(!url) {
    GST_DEBUG("no url yet, not opening");
    return;
  }

  if(opened) {
    GST_ERROR("changing width,height,url not yet supported");
    return;
  }

  new_browser(cef);
}

void
gst_cef_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCef *cef = GST_CEF (object);
  GST_OBJECT_LOCK(cef);

  GST_DEBUG_OBJECT (cef, "set_property");

  switch (property_id) {
    case PROP_URL:
      {
        const gchar *url;
        url = g_value_get_string (value);
        g_free (cef->url);
        cef->url = g_strdup (url);
        try_new_browser(cef);
        break;
      }
    case PROP_WIDTH:
      {
        const width = g_value_get_uint (value);
        cef->width = width;
        try_new_browser(cef);
        break;
      }
    case PROP_HEIGHT:
      {
        const height = g_value_get_uint (value);
        cef->height = height;
        try_new_browser(cef);
        break;
      }
    case PROP_CLEANUP:
      {
        cef->cleanup = TRUE;
        cleanup(cef);
        break;
      }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }

  GST_OBJECT_UNLOCK(cef);
}

void gst_cef_set_size(GstCef *cef, int width, int height) {
  struct gstSizeArgs *args = g_malloc(sizeof(struct gstSizeArgs));

  GST_INFO("actual new browser");
  args->gstCef = cef;
  args->width = width;
  args->height = height;

  set_size(args);
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
    case PROP_CLEANUP:
      g_value_set_boolean(value, cef->cleanup);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
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

