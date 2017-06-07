
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

#include "include/base/cef_logging.h"
#include <X11/Xlib.h>
namespace {

int XErrorHandlerImpl(Display *display, XErrorEvent *event) {
  LOG(WARNING)
        << "X error received: "
        << "type " << event->type << ", "
        << "serial " << event->serial << ", "
        << "error_code " << static_cast<int>(event->error_code) << ", "
        << "request_code " << static_cast<int>(event->request_code) << ", "
        << "minor_code " << static_cast<int>(event->minor_code);
  return 0;
}

int XIOErrorHandlerImpl(Display *display) {
  return 0;
}

static gint loop_live = 0;
static CefRefPtr<Browser> app;
static GMutex cef_start_mutex;
static GCond cef_start_cond;

}  // namespace

static void doStart(gpointer data) {
  std::cout << "doStart" << std::endl;
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
  app = new Browser(cb->gstCef, cb->push_frame, cb->url, 1280, 720);
  cb->url = NULL;
  g_free(cb);

  // Initialize CEF for the browser process.
  std::cout << "CefInitialize" << std::endl;
  CefInitialize(main_args, settings, app.get(), NULL);

  g_cond_signal(&cef_start_cond);
  g_mutex_unlock(&cef_start_mutex);
}

static bool doOpen(gpointer data) {
  struct gstCb *cb = (struct gstCb*) data;
  app.get()->Open(cb->gstCef, cb->push_frame, cb->url);
  cb->url = NULL;
  /* g_free(cb); */
  return false;
}

static bool doWork(gpointer data) {
  /* std::cout << "doWork" << std::endl; */
  if (g_atomic_int_get(&loop_live)) {
      /* std::cout << "Calling CefDoMessageLoopWork" << std::endl; */
      CefDoMessageLoopWork();
  }
  return false;
}

static bool doShutdown(gpointer data) {
  std::cout << "Calling CefShutdown" << std::endl;
  CefShutdown();
  std::cout << "Called CefShutdown" << std::endl;
  return false;
}

void browser_loop(gpointer args) {
  // TODO: this wont work
  if (app) {
    g_idle_add((GSourceFunc) doOpen, args);
    return;
  }

  std::cout << "starting browser_loop" << std::endl;

  g_atomic_int_set(&loop_live, 1);

  gstCb *cb = (gstCb*) args;

  g_mutex_lock(&cef_start_mutex);

  g_idle_add((GSourceFunc) doStart, args);

  // Run the CEF message loop. This will block until CefQuitMessageLoop() is
  usleep(100000);
  while(g_atomic_int_get(&loop_live)) {
    usleep(5000);
    g_idle_add((GSourceFunc) doWork, NULL);
  }
  usleep(5000);
  std::cout << "MessageLoop Ended" << std::endl;
  /* g_idle_add((GSourceFunc) doShutdown, NULL); */

}

void doOpenBrowser(gpointer args) {
  g_mutex_lock(&cef_start_mutex);
  while(!app) {
    std::cout << "waiting" << std::endl;
    g_cond_wait(&cef_start_cond, &cef_start_mutex);
  }
  g_mutex_unlock(&cef_start_mutex);
  g_idle_add((GSourceFunc) doOpen, args);
}

void open_browser(gpointer args) {
  g_thread_new("open_browser", (GThreadFunc) doOpenBrowser, args);
}

bool doCloseAll(gpointer data) {
    if(app) {
        app.get()->CloseAllBrowsers(true);
    } else {
        std::cout << "ERROR: no app" << std::endl;
    }
    return false;
}

void close_all_browsers() {
  g_idle_add((GSourceFunc) doCloseAll, NULL);
}

void shutdown_browser() {
  g_atomic_int_set(&loop_live, 0);
  usleep(3000000);
  g_idle_add((GSourceFunc) doShutdown, NULL);
}
