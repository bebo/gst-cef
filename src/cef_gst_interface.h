#ifndef _CEF_H_
#define _CEF_H_

#include <stdint.h>
#include <gst/gstinfo.h>

#ifdef __cplusplus
extern "C" {
#endif
GST_EXPORT GstDebugCategory *gst_cef_debug_category;
#define GST_CAT_DEFAULT gst_cef_debug_category

struct gstCb
{
  void *gstCef;
  void *push_frame;
  char *url;
  char *initialization_js;
  char *bebofile_path;
  int width;
  int height;
};

struct gstSizeArgs
{
  void *gstCef;
  int width;
  int height;
};

struct gstHiddenArgs
{
  void *gstCef;
  gboolean hidden;
};

struct gstExecuteJSArgs
{
  void *gstCef;
  char *js;
};

void browser_loop(gpointer args);
void open_browser(gpointer args);
void set_size(void *args);
void set_hidden(void *args);
void execute_js(void *args);
void set_initialization_js(void *args);
void close_browser(gpointer args);
void shutdown_browser();

#ifdef __cplusplus
}
#endif

#endif
