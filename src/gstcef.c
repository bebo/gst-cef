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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include "gstcef.h"
#include "cef.h"

GST_DEBUG_CATEGORY(gst_cef_debug_category);
#define DEFAULT_IS_LIVE TRUE

static void gst_cef_set_property(GObject *object,
                                 guint property_id, const GValue *value, GParamSpec *pspec);
static void gst_cef_get_property(GObject *object,
                                 guint property_id, GValue *value, GParamSpec *pspec);
static void gst_cef_dispose(GObject *object);
static void gst_cef_finalize(GObject *object);

static GstCaps *gst_cef_get_caps(GstBaseSrc *src, GstCaps *filter);
static gboolean gst_cef_is_seekable(GstBaseSrc *src);
static gboolean gst_cef_unlock(GstBaseSrc *src);
static gboolean gst_cef_unlock_stop(GstBaseSrc *src);
static GstFlowReturn gst_cef_create(GstPushSrc *src, GstBuffer **buf);
static gboolean gst_cef_start(GstBaseSrc *src);
static gboolean gst_cef_stop(GstBaseSrc *src);

// https://bebo.com is way too heavy to use as the default.
#define DEFAULT_URL "https://google.com"
#define DEFAULT_HEIGHT 720
#define DEFAULT_WIDTH 1280
#define DEFAULT_JS ""
#define DEFAULT_INITIALIZATION_JS ""

enum
{
  PROP_0,
  PROP_URL,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_HIDDEN,
  PROP_JS,
  PROP_INIT_JS
};

/* pad templates */
#define VTS_VIDEO_CAPS GST_VIDEO_CAPS_MAKE("BGRA")

static GstStaticPadTemplate gst_cef_src_template =
    GST_STATIC_PAD_TEMPLATE("src",
                            GST_PAD_SRC,
                            GST_PAD_ALWAYS,
                            GST_STATIC_CAPS(VTS_VIDEO_CAPS));

/* class initialization */

static GstElementClass *parent_element_class = NULL;

G_DEFINE_TYPE_WITH_CODE(GstCef, gst_cef, GST_TYPE_PUSH_SRC,
                        GST_DEBUG_CATEGORY_INIT(gst_cef_debug_category, "cef", 0,
                                                "debug category for cef element"));

static GThread *browserLoop;

/* 
 * Initialization function that is called once.
 */
static void
gst_cef_class_init(GstCefClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  printf("gst_cef_class_init\n");
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS(klass);
  GstPushSrcClass *push_src_class = GST_PUSH_SRC_CLASS(klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS(klass);
  parent_element_class = element_class;

  gst_element_class_add_static_pad_template(GST_ELEMENT_CLASS(klass),
                                            &gst_cef_src_template);

  gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
                                        "Gstreamer chromium embedded (cef)", "Generic", "FIXME Description",
                                        "Florian P. Nierhaus <fpn@bebo.com>");

  gobject_class->set_property = gst_cef_set_property;
  gobject_class->get_property = gst_cef_get_property;

  gobject_class->dispose = gst_cef_dispose;
  base_src_class->get_caps = GST_DEBUG_FUNCPTR(gst_cef_get_caps);
  base_src_class->is_seekable = GST_DEBUG_FUNCPTR(gst_cef_is_seekable);
  base_src_class->unlock = GST_DEBUG_FUNCPTR(gst_cef_unlock);
  base_src_class->unlock_stop = GST_DEBUG_FUNCPTR(gst_cef_unlock_stop);
  base_src_class->start = GST_DEBUG_FUNCPTR(gst_cef_start);
  base_src_class->stop = GST_DEBUG_FUNCPTR(gst_cef_stop);
  push_src_class->create = GST_DEBUG_FUNCPTR(gst_cef_create);

  g_object_class_install_property(gobject_class, PROP_URL,
                                  g_param_spec_string("url", "url", "website to render into video",
                                                      DEFAULT_URL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property(gobject_class, PROP_WIDTH,
                                  g_param_spec_uint("width", "width", "website to render into video",
                                                    0, G_MAXUINT, DEFAULT_WIDTH, G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class, PROP_HEIGHT,
                                  g_param_spec_uint("height", "height", "website to render into video",
                                                    0, G_MAXUINT, DEFAULT_HEIGHT, G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class, PROP_HIDDEN,
                                  g_param_spec_boolean("hidden", "hidden", "set the cef browser to hidden for throttling",
                                                       FALSE, G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class, PROP_INIT_JS,
                                  g_param_spec_string("initialization-js", "initialization-js",
                                                      "JavaScript to be initialized OnLoadEnd",
                                                      DEFAULT_INITIALIZATION_JS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property(gobject_class, PROP_JS,
                                  g_param_spec_string("javascript", "javascript", "javascript to be executed by window.",
                                                      DEFAULT_JS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void push_frame(void *gstCef, const void *buffer, int width, int height)
{
  GST_DEBUG("Pushing Frame");
  GstCef *cef = (GstCef *)gstCef;
  int size = width * height * 4;
  if (size != (cef->width * cef->height * 4))
  {
    GST_ERROR("push_frame size mismatch");
  }

  g_mutex_lock(&cef->frame_mutex);

  if (cef->current_buffer)
  {
    memcpy(cef->current_buffer, buffer, size);
    g_atomic_int_set(&cef->has_new_frame, 1);
    g_cond_signal(&cef->frame_cond);
    g_mutex_unlock(&cef->frame_mutex);
    return;
  }

  g_mutex_unlock(&cef->frame_mutex);
}

void *pop_frame(GstCef *cef)
{

  gint64 end_time;
  // Send at least one frame per 20s.
  end_time = g_get_monotonic_time() + 20000 * G_TIME_SPAN_MILLISECOND;
  while (g_atomic_int_get(&cef->has_new_frame) == 0 && g_atomic_int_get(&cef->unlocked) == 0)
  {
    if (!g_cond_wait_until(&cef->frame_cond, &cef->frame_mutex, end_time))
    {
      break;
    }
  }

  if (g_atomic_int_get(&cef->unlocked) == 0)
  { // 0 - not in cleanup state
    g_atomic_int_set(&cef->has_new_frame, 0);
    return cef->current_buffer;
  }

  return NULL;
}

void new_browser(GstCef *cef)
{
  const GstStructure *structure;
  struct gstCb *cb = g_malloc(sizeof(struct gstCb));

  GST_INFO("actual new browser");

  cb->gstCef = cef;
  cb->push_frame = push_frame;
  cb->url = g_strdup(cef->url);
  cb->width = cef->width;
  cb->height = cef->height;
  cb->initialization_js = cef->initialization_js;
  GST_INFO("set cb");

  if (browserLoop == 0)
  {
    GST_INFO("making browser loop");
    browserLoop = g_thread_ref(g_thread_new("browser_loop", (GThreadFunc)browser_loop, cb));
  }
  else
  {
    GST_INFO("open browser");
    open_browser(cb);
  }
}

/*
 * Used to initialize a specific instance of cef.
 */
void gst_cef_init(GstCef *cef)
{
  printf("gst_cef_init\n");

  g_atomic_int_set(&cef->unlocked, 0);
  cef->has_opened_browser = FALSE;
  cef->width = DEFAULT_WIDTH;
  cef->height = DEFAULT_HEIGHT;
  cef->url = g_strdup(DEFAULT_URL);
  cef->initialization_js = g_strdup(DEFAULT_INITIALIZATION_JS);
  cef->hidden = FALSE;
  g_mutex_init(&cef->frame_mutex);
  g_cond_init(&cef->frame_cond);

  gst_base_src_set_format(GST_BASE_SRC(cef), GST_FORMAT_TIME);
  gst_base_src_set_live(GST_BASE_SRC(cef), DEFAULT_IS_LIVE);
  gst_base_src_set_do_timestamp(GST_BASE_SRC(cef), TRUE);
}

void gst_cef_set_hidden(GstCef *cef, gboolean hidden)
{
  struct gstHiddenArgs *args = g_malloc(sizeof(struct gstHiddenArgs));
  args->gstCef = cef;
  args->hidden = hidden;
  set_hidden(args);
}

void gst_cef_execute_js(GstCef *cef)
{
  struct gstExecuteJSArgs *args = g_malloc(sizeof(struct gstExecuteJSArgs));
  args->gstCef = cef;
  args->js = g_strdup(cef->js);
  execute_js(args);
}

void gst_cef_set_property(GObject *object, guint property_id,
                          const GValue *value, GParamSpec *pspec)
{
  GstCef *cef = GST_CEF(object);

  GST_DEBUG_OBJECT(cef, "set_property");

  switch (property_id)
  {
  case PROP_URL:
  {
    const gchar *url = g_value_get_string(value);
    g_free(cef->url);
    cef->url = g_strdup(url);
    break;
  }
  case PROP_WIDTH:
  {
    const guint width = g_value_get_uint(value);
    cef->width = width;
    break;
  }
  case PROP_HEIGHT:
  {
    const guint height = g_value_get_uint(value);
    cef->height = height;
    break;
  }
  case PROP_JS:
  {
    const gchar *js = g_value_get_string(value);
    g_free(cef->js);
    cef->js = g_strdup(js);
    gst_cef_execute_js(cef);
    break;
  }
  case PROP_INIT_JS:
  {
    const gchar *init_js = g_value_get_string(value);
    g_free(cef->initialization_js);
    cef->initialization_js = g_strdup(init_js);
    break;
  }
  case PROP_HIDDEN:
  {
    const gboolean hidden = g_value_get_boolean(value);
    if (hidden != cef->hidden)
    {
      cef->hidden = hidden;
      gst_cef_set_hidden(cef, hidden);
    }
    break;
  }
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

void gst_cef_set_size(GObject *object, int width, int height)
{
  GstCef *cef = GST_CEF(object);
  struct gstSizeArgs *args = g_malloc(sizeof(struct gstSizeArgs));
  args->gstCef = cef;
  args->width = width;
  args->height = height;

  set_size(args);
  GST_INFO("setting size");
}

void gst_cef_get_property(GObject *object, guint property_id,
                          GValue *value, GParamSpec *pspec)
{
  GstCef *cef = GST_CEF(object);

  GST_DEBUG_OBJECT(cef, "get_property");
  switch (property_id)
  {
  case PROP_URL:
    g_value_set_string(value, cef->url);
    break;
  case PROP_WIDTH:
    g_value_set_uint(value, cef->width);
    break;
  case PROP_HEIGHT:
    g_value_set_uint(value, cef->height);
    break;
  case PROP_HIDDEN:
    g_value_set_boolean(value, cef->hidden);
    break;
  case PROP_JS:
    g_value_set_string(value, cef->js);
    break;
  case PROP_INIT_JS:
    g_value_set_string(value, cef->initialization_js);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

void gst_cef_dispose(GObject *object)
{
  GstCef *cef = GST_CEF(object);
  GST_INFO("gst_cef_dispose");
  GST_DEBUG_OBJECT(cef, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS(gst_cef_parent_class)->dispose(object);
}

void gst_cef_finalize(GObject *object)
{
  GstCef *cef = GST_CEF(object);
  GST_INFO("gst_cef_finalize");
  GST_DEBUG_OBJECT(cef, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS(gst_cef_parent_class)->finalize(object);
}

/* get caps from subclass */
static GstCaps *
gst_cef_get_caps(GstBaseSrc *src, GstCaps *filter)
{
  GstCef *cef = GST_CEF(src);
  GstCaps *caps;

  GST_DEBUG_OBJECT(cef, "get_caps");

  caps = gst_caps_new_simple("video/x-raw",
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
gst_cef_is_seekable(GstBaseSrc *src)
{
  return FALSE;
}

static gboolean
gst_cef_unlock(GstBaseSrc *src)
{
  GstCef *cef = GST_CEF(src);

  GST_INFO_OBJECT(cef, "unlock");

  g_mutex_lock(&cef->frame_mutex);

  close_browser(cef);

  g_atomic_int_set(&cef->unlocked, 1);
  g_cond_signal(&cef->frame_cond);

  g_mutex_unlock(&cef->frame_mutex);

  GST_INFO_OBJECT(cef, "unlock complete");
  return TRUE;
}

static gboolean gst_cef_start(GstBaseSrc *src)
{
  GstCef *cef = GST_CEF(src);
  gint width = cef->width;
  gint height = cef->width;
  char *url = cef->url;

  GST_INFO_OBJECT(cef, "start");

  if (!width || !height || !url)
  {
    GST_ERROR("no width, or height, or url");
    return FALSE;
  }

  gsize size = 4 * cef->width * cef->height;
  cef->current_buffer = g_malloc(size);
  memset(cef->current_buffer, 0, size);

  new_browser(cef);

  return TRUE;
}

static gboolean gst_cef_stop(GstBaseSrc *src)
{
  GstCef *cef = GST_CEF(src);
  GST_INFO_OBJECT(cef, "stop");
  close_browser(cef);
  if (cef->current_buffer)
  {
    g_free(cef->current_buffer);
    cef->current_buffer = NULL;
  }
  return TRUE;
}

/* Clear any pending unlock request, as we succeeded in unlocking */
static gboolean
gst_cef_unlock_stop(GstBaseSrc *src)
{
  GstCef *cef = GST_CEF(src);
  gint width = cef->width;
  gint height = cef->width;
  char *url = cef->url;

  GST_INFO_OBJECT(cef, "unlock_stop");

  g_mutex_lock(&cef->frame_mutex);

  if (!width || !height || !url)
  {
    GST_ERROR("no width, or height, or url");
    return FALSE;
  }

  g_atomic_int_set(&cef->unlocked, 0);

  g_mutex_unlock(&cef->frame_mutex);

  GST_INFO_OBJECT(cef, "unlock_stop complete");
  return TRUE;
}

static GstFlowReturn gst_cef_create(GstPushSrc *src, GstBuffer **buf)
{
  GstCef *cef = GST_CEF(src);

  g_mutex_lock(&cef->frame_mutex);
  GST_DEBUG("Popping Cef Frame");
  void *frame = pop_frame(cef);
  if (!frame)
  {
    GST_DEBUG("No frame returned");
    g_mutex_unlock(&cef->frame_mutex);
    return GST_FLOW_FLUSHING;
  }
  GST_DEBUG("Successfully popped frame.");

  gsize my_size = cef->width * cef->height * 4;
  GstBuffer *buffer = gst_buffer_new_allocate(NULL, my_size, NULL);
  gst_buffer_fill(buffer, 0, frame, my_size);
  *buf = buffer;
  g_mutex_unlock(&cef->frame_mutex);
  return GST_FLOW_OK;
}

static gboolean
plugin_init(GstPlugin *plugin)
{

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register(plugin, "cef", GST_RANK_NONE,
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

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
                  GST_VERSION_MINOR,
                  cef,
                  "Chromium Embedded src plugin",
                  plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
