#include <sys/auxv.h>

#include <ctime>
#include <iostream>
#include <list>
#include <glib.h>
#include <unistd.h>
#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/cef_render_handler.h>
#include <include/wrapper/cef_helpers.h>

#include "cef.h"
#include "cef_app.h"

#include <X11/Xlib.h>


namespace {

int XErrorHandlerImpl(Display *display, XErrorEvent *event) {
  GST_ERROR("X error received: "
            "type %d "
            "serial %lu "
            "error_code %d "
            "request_code %d "
            "minor_code %d",
            event->type,
            event->serial,
            static_cast<int>(event->error_code),
            static_cast<int>(event->request_code),
            static_cast<int>(event->minor_code));
  return 0;
}

int XIOErrorHandlerImpl(Display *display) {
  return 0;
}

static gint loop_live = 0;
static CefRefPtr<Browser> app;
static GMutex cef_start_mutex;
static GCond cef_start_cond;
static guint browser_loop_index = 0;

}  // namespace

static void doStart(gpointer data) {
  GST_LOG("doStart");

  // Provide CEF with command-line arguments.
  struct gstCb *cb = (struct gstCb*) data;
  CefMainArgs main_args(0, nullptr);

  // Install xlib error handlers so that the application won't be terminated
  // on non-fatal errors.
  XInitThreads();
  XSetErrorHandler(XErrorHandlerImpl);
  XSetIOErrorHandler(XIOErrorHandlerImpl);

  // Specify CEF global settings here.
  CefSettings settings;

  char * exe = (char *) getauxval(AT_EXECFN);
  gchar * dirname = g_path_get_dirname(exe);
  gchar * subprocess_exe = g_strconcat(dirname, "/subprocess", NULL);
  g_free(dirname);

  CefString(&settings.browser_subprocess_path).FromASCII(subprocess_exe);
  g_free(subprocess_exe);
  settings.windowless_rendering_enabled = true;
  settings.no_sandbox = false;
  settings.multi_threaded_message_loop = false;

  // Browser implements application-level callbacks for the browser process.
  // It will create the first browser instance in OnContextInitialized() after
  // CEF has initialized.
  app = new Browser(cb->gstCef, cb->push_frame, cb->url, cb->width, cb->height);
  g_free(cb);

  // Initialize CEF for the browser process.
  GST_LOG("CefInitialize");
  CefInitialize(main_args, settings, app.get(), NULL);

  g_cond_signal(&cef_start_cond);
  g_mutex_unlock(&cef_start_mutex);
}

static bool doOpen(gpointer data) {
  GST_INFO("doOpen");
  struct gstCb *cb = (struct gstCb*) data;
  app.get()->Open(cb->gstCef, cb->push_frame, cb->url, cb->width, cb->height);

  //FIXME: figure out why we can't free cb
  /* g_free(cb); */
  return false;
}

static bool doWork(gpointer data) {
  if (g_atomic_int_get(&loop_live)) {
      CefDoMessageLoopWork();
  }
  return false;
}

static bool doShutdown(gpointer data) {
  GST_LOG("doShutdown");
  CefShutdown();
  return false;
}

void browser_loop(gpointer args) {
  // TODO: this wont work
  if (app) {
    GST_INFO("have app");
    g_idle_add((GSourceFunc) doOpen, args);
    return;
  }

  browser_loop_index++;

  g_atomic_int_set(&loop_live, 1);

  g_mutex_lock(&cef_start_mutex);

  g_idle_add((GSourceFunc) doStart, args);

  // Run the CEF message loop. This will block until CefQuitMessageLoop() is
  usleep(100000);
  while(g_atomic_int_get(&loop_live)) {
    usleep(20000);
    g_idle_add((GSourceFunc) doWork, NULL);
  }
  usleep(5000);
  GST_INFO("MessageLoop Ended");
}

void doOpenBrowser(gpointer args) {
  GST_INFO("doOpenBrowser");
  g_mutex_lock(&cef_start_mutex);
  while(!app) {
    g_cond_wait(&cef_start_cond, &cef_start_mutex);
  }
  g_mutex_unlock(&cef_start_mutex);
  g_idle_add((GSourceFunc) doOpen, args);
}

void open_browser(gpointer args) {
  g_thread_new("open_browser", (GThreadFunc) doOpenBrowser, args);
}

static bool doSetSize(void *args) {
  gstSizeArgs *sizeArgs = (gstSizeArgs *)args;
  int width = sizeArgs->width;
  int height = sizeArgs->height;
  void *gstCef = sizeArgs->gstCef;
  g_free(args);
  GST_INFO("set size to %u by %u", width, height);

  if(!app) {
    return false;
  }

  app->SetSize(gstCef, width, height);
  return false;
}

void set_size(void *args) {
  GST_INFO("set_size");
  g_idle_add((GSourceFunc) doSetSize, args);
}

static bool doSetHidden(void *args) {
  gstHiddenArgs *hiddenArgs = (gstHiddenArgs *)args;
  bool hidden = hiddenArgs->hidden;
  void *gstCef = hiddenArgs->gstCef;
  g_free(args);

  if(!app) {
    return false;
  }

  app->SetHidden(gstCef, hidden);
  return false;
}

void set_hidden(void * args) {
  GST_INFO("set_hidden");
  g_idle_add((GSourceFunc) doSetHidden, args);
}

bool doClose(gpointer args) {
  if(app) {
    app->CloseBrowser(args, true);
  } else {
        GST_ERROR("ERROR: no app");
  }
  return false;
}

void close_browser(gpointer args) {
  g_idle_add((GSourceFunc) doClose, args);
}

void shutdown_browser() {
  GST_WARNING("shutdown browser");
  g_atomic_int_set(&loop_live, 0);
  usleep(3000000);
  g_idle_add((GSourceFunc) doShutdown, NULL);
}
