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

GST_DEBUG_CATEGORY_STATIC (gst_cef_debug_category);
#define GST_CAT_DEFAULT gst_cef_debug_category
#define DEFAULT_IS_LIVE            TRUE

/* prototypes */


static void gst_cef_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_cef_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_cef_dispose (GObject * object);
static void gst_cef_finalize (GObject * object);

static GstCaps *gst_cef_get_caps (GstBaseSrc * src, GstCaps * filter);
static gboolean gst_cef_negotiate (GstBaseSrc * src);
static GstCaps *gst_cef_fixate (GstBaseSrc * src, GstCaps * caps);
static gboolean gst_cef_set_caps (GstBaseSrc * src, GstCaps * caps);
static gboolean gst_cef_decide_allocation (GstBaseSrc * src,
    GstQuery * query);
static gboolean gst_cef_start (GstBaseSrc * src);
static gboolean gst_cef_stop (GstBaseSrc * src);
static void gst_cef_get_times (GstBaseSrc * src, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end);
static gboolean gst_cef_get_size (GstBaseSrc * src, guint64 * size);
static gboolean gst_cef_is_seekable (GstBaseSrc * src);
/* static gboolean gst_cef_prepare_seek_segment (GstBaseSrc * src, */
    /* GstEvent * seek, GstSegment * segment); */
/* static gboolean gst_cef_do_seek (GstBaseSrc * src, GstSegment * segment); */
static gboolean gst_cef_unlock (GstBaseSrc * src);
static gboolean gst_cef_unlock_stop (GstBaseSrc * src);
static gboolean gst_cef_query (GstBaseSrc * src, GstQuery * query);
static gboolean gst_cef_event (GstBaseSrc * src, GstEvent * event);
static GstFlowReturn gst_cef_create (GstBaseSrc * src, guint64 offset,
    guint size, GstBuffer ** buf);
static GstFlowReturn gst_cef_alloc (GstBaseSrc * src, guint64 offset,
    guint size, GstBuffer ** buf);
static GstFlowReturn gst_cef_fill (GstBaseSrc * src, guint64 offset,
    guint size, GstBuffer * buf);

enum
{
  PROP_0,
  PROP_VERBOSE,
  PROP_URL
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

G_DEFINE_TYPE_WITH_CODE (GstCef, gst_cef, GST_TYPE_PUSH_SRC,
  GST_DEBUG_CATEGORY_INIT (gst_cef_debug_category, "cef", 0,
  "debug category for cef element"));
void * browser = NULL;
pthread_t browserMessageLoop = 0;

static void
gst_cef_class_init (GstCefClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  printf("gst_cef_class_init\n");
  GstBaseSrcClass *base_src_class = GST_BASE_SRC_CLASS (klass);

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS(klass),
      &gst_cef_src_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS(klass),
      "Gstreamer chromium embedded (cef)", "Generic", "FIXME Description",
      "Florian P. Nierhaus <fpn@bebo.com>");

  gobject_class->set_property = gst_cef_set_property;
  gobject_class->get_property = gst_cef_get_property;

  /* base_src_class->fixate = GST_DEBUG_FUNCPTR (gst_cef_fixate); */
  /* base_src_class->set_caps = GST_DEBUG_FUNCPTR (gst_cef_set_caps); */

  /* gobject_class->dispose = gst_cef_dispose; */
  gobject_class->finalize = gst_cef_finalize;
  base_src_class->get_caps = GST_DEBUG_FUNCPTR (gst_cef_get_caps);
  /* base_src_class->negotiate = GST_DEBUG_FUNCPTR (gst_cef_negotiate); */
  base_src_class->is_seekable = GST_DEBUG_FUNCPTR (gst_cef_is_seekable);
  /* base_src_class->fill = GST_DEBUG_FUNCPTR (gst_cef_fill); */

  /* base_src_class->decide_allocation = GST_DEBUG_FUNCPTR (gst_cef_decide_allocation); */
  /* base_src_class->start = GST_DEBUG_FUNCPTR (gst_cef_start); */
  /* base_src_class->stop = GST_DEBUG_FUNCPTR (gst_cef_stop); */
  /* base_src_class->get_times = GST_DEBUG_FUNCPTR (gst_cef_get_times); */
  base_src_class->get_size = GST_DEBUG_FUNCPTR (gst_cef_get_size);
  /* /1* base_src_class->prepare_seek_segment = GST_DEBUG_FUNCPTR (gst_cef_prepare_seek_segment); *1/ */
  /* /1* base_src_class->do_seek = GST_DEBUG_FUNCPTR (gst_cef_do_seek); *1/ */
  /* base_src_class->unlock = GST_DEBUG_FUNCPTR (gst_cef_unlock); */
  /* base_src_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_cef_unlock_stop); */
  /* base_src_class->query = GST_DEBUG_FUNCPTR (gst_cef_query); */
  /* base_src_class->event = GST_DEBUG_FUNCPTR (gst_cef_event); */
  base_src_class->create = GST_DEBUG_FUNCPTR (gst_cef_create);
  /* base_src_class->alloc = GST_DEBUG_FUNCPTR (gst_cef_alloc); */
  /* base_src_class->alloc = NULL; */
  /* base_src_class->fill = NULL; */

  g_object_class_install_property (gobject_class, PROP_VERBOSE,
      g_param_spec_boolean ("verbose", "Verbose", "Produce verbose output",
          FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_URL,
      g_param_spec_string ("url", "URL", "website to render into video",
        "https://bebo.com/", G_PARAM_READWRITE));
}

static void push_frame(void *gstCef, const void *buffer, int width, int height) {
  GstClock *clock = gst_system_clock_obtain();
  GstClockTime now = gst_clock_get_time(clock);

  GstCef *cef = (GstCef *) gstCef;
  int size = width * height * 4 * 1;
  GstBuffer *buf;
  buf = gst_buffer_new_allocate (NULL, size, NULL);
  if (G_UNLIKELY (buf == NULL)) {
    GST_ERROR_OBJECT (cef, "Failed to allocate %u bytes", size);
    return;
  }
  if (cef->startTime == 0) {
    cef->startTime = now;
  }

  guint8 *data;
  GstMapInfo map;
  gst_buffer_map(buf, &map, GST_MAP_WRITE);
  memcpy(map.data, buffer, size);
  gst_buffer_unmap (buf, &map);
  GST_BUFFER_TIMESTAMP (buf) = now - cef->startTime; // live sources should start at 0
  GST_BUFFER_OFFSET (buf) = 0;
  if (now == cef->startTime) {
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DISCONT);
  }
  g_mutex_lock(&cef->frame_mutex);
  if (cef->current_frame != NULL) {
    gst_buffer_unref(cef->current_frame);
  }
  cef->current_frame = buf;
  g_cond_signal (&cef->frame_cond);
  g_mutex_unlock (&cef->frame_mutex);
}

gpointer pop_frame(GstCef *cef)
{
  gpointer frame;
  g_mutex_lock (&cef->frame_mutex);
  while (!cef->current_frame) {
    g_cond_wait (&cef->frame_cond, &cef->frame_mutex);
  }
  frame = cef->current_frame;
  cef->current_frame= NULL;
  g_mutex_unlock (&cef->frame_mutex);
  return frame;
}

void new_browser(GstCef *cef) {
    if (cef->browserLoop == 0) {
        /*     /1* new_browser(&browser, cef->url, 1280, 720, 30, NULL); *1/ */
        /* pthread_attr_t attr; */
        /* /1* Initialize and set thread detached attribute *1/ */
        /* pthread_attr_init(&attr); */
        /* pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE); */
        /* pthread_create(&browserMessageLoop, &attr, (void *) browser_loop, NULL); */
        /* /1* browser_loop(NULL); *1/ */
        /* pthread_join(browserMessageLoop, NULL); */
        /* GThread * */
        struct gstCb *cb = g_malloc(sizeof(struct gstCb));
        cb->gstCef = cef;
        cb->push_frame = push_frame;
        cb->url = g_strdup(cef->url);
        cef->browserLoop = g_thread_ref(g_thread_new("browser_loop", (GThreadFunc)browser_loop, cb));
    }
}

void gst_cef_init(GstCef *cef)
{
  printf("gst_cef_init\n");

  gst_base_src_set_format (GST_BASE_SRC (cef), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (cef), DEFAULT_IS_LIVE);

}

void set_url(GstCef *cef, char * url) {
  cef->url = url;
  if (cef->browserLoop == 0) {
      new_browser(cef);
  } else {
    // FIXME tell chromium to change url
  }
}

void
gst_cef_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCef *cef = GST_CEF (object);

  GST_DEBUG_OBJECT (cef, "set_property");

  switch (property_id) {
    case PROP_VERBOSE:
      cef->verbose = g_value_get_boolean (value);
      break;
    case PROP_URL:
      set_url(cef, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_cef_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstCef *cef = GST_CEF (object);

  GST_DEBUG_OBJECT (cef, "get_property");
  switch (property_id) {
    case PROP_VERBOSE:
      g_value_set_boolean(value, cef->verbose);
      break;
    case PROP_URL:
      g_value_set_string(value, cef->url);
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

  GST_DEBUG_OBJECT (cef, "get_caps");

  /* GstCaps *caps = NULL, *temp = NULL; */
  /* GstPadTemplate *pad_template; */
  /* GstBaseSrcClass *bclass = GST_BASE_SRC_GET_CLASS (cef); */
  /* guint i; */
  /* gint width, height; */

/* /1*   if (qt_src->window) { *1/ */
/* /1*     qt_src->window->getGeometry (&width, &height); *1/ */
/* /1*   } *1/ */

  /* pad_template = gst_element_class_get_pad_template (GST_ELEMENT_CLASS (cef), "src"); */
  /* if (pad_template != NULL) */
  /*   caps = gst_pad_template_get_caps (pad_template); */


  /* guint n_caps = gst_caps_get_size (caps); */
  /*   for (i = 0; i < n_caps; i++) { */
  /*     GstStructure *s = gst_caps_get_structure (caps, i); */
  /*     gst_structure_set (s, "width", G_TYPE_INT, 1280, NULL); */
  /*     gst_structure_set (s, "height", G_TYPE_INT, 720, NULL); */
  /*     /1* because the framerate is unknown *1/ */
  /*     gst_structure_set (s, "framerate", GST_TYPE_FRACTION, 0, 1, NULL); */
  /*     gst_structure_set (s, "pixel-aspect-ratio", */
  /*         GST_TYPE_FRACTION, 1, 1, NULL); */
  /*   } */

GstCaps *caps = gst_caps_new_simple ("video/x-raw",
   "format", G_TYPE_STRING, "BGRA",
   "framerate", GST_TYPE_FRACTION, 30, 1,
   "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
   "width", G_TYPE_INT, 1280,
   "height", G_TYPE_INT, 720,
   NULL);

  return caps;

}

/* decide on caps */
static gboolean
gst_cef_negotiate (GstBaseSrc * src)
{
  GstCef *cef = GST_CEF (src);

  GST_DEBUG_OBJECT (cef, "negotiate");

  return TRUE;
}

/* called if, in negotiation, caps need fixating */
static GstCaps *
gst_cef_fixate (GstBaseSrc * src, GstCaps * caps)
{
  GstCef *cef = GST_CEF (src);

  GST_DEBUG_OBJECT (cef, "fixate");

  return NULL;
}

/* notify the subclass of new caps */
static gboolean
gst_cef_set_caps (GstBaseSrc * src, GstCaps * caps)
{
  GstCef *cef = GST_CEF (src);

  GST_DEBUG_OBJECT (cef, "set_caps");

  return TRUE;
}

/* setup allocation query */
static gboolean
gst_cef_decide_allocation (GstBaseSrc * src, GstQuery * query)
{
  GstCef *cef = GST_CEF (src);

  GST_DEBUG_OBJECT (cef, "decide_allocation");

  return TRUE;
}

/* start and stop processing, ideal for opening/closing the resource */
static gboolean
gst_cef_start (GstBaseSrc * src)
{
  GstCef *cef = GST_CEF (src);

  GST_DEBUG_OBJECT (cef, "start");

  return TRUE;
}

static gboolean
gst_cef_stop (GstBaseSrc * src)
{
  GstCef *cef = GST_CEF (src);

  GST_DEBUG_OBJECT (cef, "stop");

  return TRUE;
}

/* given a buffer, return start and stop time when it should be pushed
 * out. The base class will sync on the clock using these times. */
static void
gst_cef_get_times (GstBaseSrc * src, GstBuffer * buffer,
    GstClockTime * start, GstClockTime * end)
{
  GstCef *cef = GST_CEF (src);

  GST_DEBUG_OBJECT (cef, "get_times");

}

/* get the total size of the resource in bytes */
static gboolean
gst_cef_get_size (GstBaseSrc * src, guint64 * size)
{
  GstCef *cef = GST_CEF (src);

  size = 1280 * 720 * 4;
  GST_DEBUG_OBJECT (cef, "get_size");

  return TRUE;
}

/* check if the resource is seekable */
static gboolean
gst_cef_is_seekable (GstBaseSrc * src)
{
  return FALSE;
}

/* /1* Prepare the segment on which to perform do_seek(), converting to the */
/*  * current basesrc format. *1/ */
/* static gboolean */
/* gst_cef_prepare_seek_segment (GstBaseSrc * src, GstEvent * seek, */
/*     GstSegment * segment) */
/* { */
/*   GstCef *cef = GST_CEF (src); */

/*   GST_DEBUG_OBJECT (cef, "prepare_seek_segment"); */

/*   return TRUE; */
/* } */

/* /1* notify subclasses of a seek *1/ */
/* static gboolean */
/* gst_cef_do_seek (GstBaseSrc * src, GstSegment * segment) */
/* { */
/*   GstCef *cef = GST_CEF (src); */

/*   GST_DEBUG_OBJECT (cef, "do_seek"); */

/*   return TRUE; */
/* } */

/* unlock any pending access to the resource. subclasses should unlock
 * any function ASAP. */
static gboolean
gst_cef_unlock (GstBaseSrc * src)
{
  GstCef *cef = GST_CEF (src);

  GST_DEBUG_OBJECT (cef, "unlock");

  return TRUE;
}

/* Clear any pending unlock request, as we succeeded in unlocking */
static gboolean
gst_cef_unlock_stop (GstBaseSrc * src)
{
  GstCef *cef = GST_CEF (src);

  GST_DEBUG_OBJECT (cef, "unlock_stop");

  return TRUE;
}

/* notify subclasses of a query */
static gboolean
gst_cef_query (GstBaseSrc * src, GstQuery * query)
{
  GstCef *cef = GST_CEF (src);

  GST_DEBUG_OBJECT (cef, "query");

  return TRUE;
}

/* notify subclasses of an event */
static gboolean
gst_cef_event (GstBaseSrc * src, GstEvent * event)
{
  GstCef *cef = GST_CEF (src);

  GST_DEBUG_OBJECT (cef, "event");

  return TRUE;
}

/* ask the subclass to create a buffer with offset and size, the default
 * implementation will call alloc and fill. */
static GstFlowReturn
gst_cef_create (GstBaseSrc * src, guint64 offset, guint size,
    GstBuffer ** buf)
{
  GstCef *cef = GST_CEF (src);

  *buf = (GstBuffer*) pop_frame(cef);
  GST_LOG_OBJECT (src, "Created buffer of size %u at %" G_GINT64_FORMAT
      " with timestamp %" GST_TIME_FORMAT, gst_buffer_get_size(*buf), GST_BUFFER_OFFSET (*buf),
      GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (*buf)));

  GST_DEBUG_OBJECT (cef, "create");

  return GST_FLOW_OK;
}

/* ask the subclass to allocate an output buffer. The default implementation
 * will use the negotiated allocator. */
static GstFlowReturn
gst_cef_alloc (GstBaseSrc * src, guint64 offset, guint size,
    GstBuffer ** buf)
{
  GstCef *cef = GST_CEF (src);

  GST_DEBUG_OBJECT (cef, "alloc");

  return GST_FLOW_OK;
}

/* ask the subclass to fill the buffer with data from offset and size */
static GstFlowReturn
gst_cef_fill (GstBaseSrc * src, guint64 offset, guint size, GstBuffer * buf)
{
  GstCef *cef = GST_CEF (src);

  GST_DEBUG_OBJECT (cef, "fill");

  return GST_FLOW_OK;
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

