#include <ctime>
#include <iostream>
#include <list>
#include <glib.h>
#ifdef __WIN
#include <Windows.h>
#else
#include <unistd.h>
#include <libgen.h>
#endif
#include <cwchar>
#include <string>

#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/cef_render_handler.h>
#include <include/wrapper/cef_helpers.h>

#include "browser_manager.h"
#include "cef_gst_interface.h"
#include "file_scheme_handler.h"

namespace
{
  static gint loop_live = 0;
  static CefRefPtr<Browser> app;
  static GMutex cef_start_mutex;
  static GCond cef_start_cond;
  static guint browser_loop_index = 0;
} // namespace

/* void getLogsPath(CHAR *filename) { */
/*   return "." */
/*   DWORD size = 8000; */
/*   memset(filename, 0, size); */

/*   RegKey registry(HKEY_CURRENT_USER, L"Software\\Bebo\\GameCapture", KEY_READ); */

/*   if (registry.HasValue(L"Logs")) { */
/*     std::wstring data; */
/*     registry.ReadValue(L"Logs", &data); */
/*     wsprintfA(filename, "%ls\\", data.c_str()); */
/*   } */
/*   else { */
/*     GetTempPathA(8000, filename); */
/*   } */
/* } */

static bool doStart(gpointer data)
{
  GST_DEBUG("doStart");

  // Provide CEF with command-line arguments.
  struct gstCb *cb = (struct gstCb *)data;



  // Specify CEF global settings here.
  CefSettings settings;

#ifdef __WIN
  CefMainArgs main_args(GetModuleHandle(NULL));
  WCHAR c_path[MAX_PATH + 50];
  GetModuleFileNameW(hModule, c_path, MAX_PATH);

  std::wstring path(c_path);
  path = path.substr(0, path.find_last_of(L"\\") + 1);

  // std::wcout << L"Resource Directory Path: " << path << std::endl;
  CefString(&settings.resources_dir_path).FromWString(path);
  std::wstring subprocess = L"\\" + SUBPROCESS_NAME;
  path = path.append(subprocess);
  // std::wcout << L"Subprocess Path: " << path << std::endl;
  CefString(&settings.browser_subprocess_path).FromWString(path);
#else
  CefMainArgs main_args(0, NULL);
  char buff[1024];
  ssize_t size = readlink("/proc/self/exe", buff, sizeof(buff));
  g_assert(size > 0);
  char * dir = dirname(buff);
  std::string path(buff);
  const char * process_name = SUBPROCESS_NAME;
  std::string subprocess = "/";
  subprocess.append(process_name);
  path = path.append(subprocess);
  CefString(&settings.browser_subprocess_path).FromString(path);

#endif

  CefString(&settings.cache_path).FromString(cb->cache_path);
  /* CHAR log_path[8000]; */
  /* getLogsPath(log_path); */
  /* strcat(log_path, "bebo_cef.log"); */
  /* GST_INFO("Cef log path: %s", log_path); */
  /* CefString(&settings.log_file).FromASCII(log_path); */

  settings.windowless_rendering_enabled = true;
  settings.no_sandbox = true;
  GST_INFO("Disabling logging.");
  settings.log_severity = LOGSEVERITY_DISABLE;
  settings.multi_threaded_message_loop = false;

  // Browser implements application-level callbacks for the browser process.
  // It will create the first browser instance in OnContextInitialized() on the 
  // browser UI thread after the CEF has initialized.
  app = new Browser();

  CefString url;
  CefString js;
  CefString local_filepath;
  url.FromASCII(cb->url);
  js.FromASCII(cb->initialization_js);
  local_filepath.FromASCII(cb->local_filepath);
  // The window will not actually be opened until the browser finishes initializing.
  app->Open(cb->gstCef, cb->push_frame, url, cb->width, cb->height, js);
  g_free(cb->url);
  g_free(cb->initialization_js);
  g_free(cb->local_filepath);
  g_free(cb->cache_path);
  g_free(cb);

  GST_DEBUG("CefInitialize");
  // Initialize CEF for the browser process.  This marks the current thread as the UI thread.
  CefInitialize(main_args, settings, app.get(), NULL);
  CefRegisterSchemeHandlerFactory(kFileSchemeProtocol, "", new FileSchemeHandlerFactory(local_filepath));

  g_mutex_lock(&cef_start_mutex);
  g_cond_signal(&cef_start_cond);
  g_mutex_unlock(&cef_start_mutex);
  return false;
}

static bool doOpen(gpointer data)
{
  GST_INFO("doOpen");
  struct gstCb *cb = (struct gstCb *)data;
  CefString url;
  CefString js;
  url.FromASCII(cb->url);
  js.FromASCII(cb->initialization_js);
  app.get()->Open(cb->gstCef, cb->push_frame, url, cb->width, cb->height, js);
  g_free(cb->initialization_js);
  g_free(cb->url);
  g_free(cb);
  return false;
}

static bool doWork(gpointer data)
{
  if (g_atomic_int_get(&loop_live))
  {
    CefDoMessageLoopWork();
  }
  return false;
}

static bool doShutdown(gpointer data)
{
  GST_LOG("doShutdown");
  CefShutdown();
  return false;
}

void browser_loop(gpointer args)
{
  GST_INFO("Entering browser loop.");
  if (app)
  {
    GST_INFO("have app");
    g_idle_add((GSourceFunc)doOpen, args);
    return;
  }
  browser_loop_index++;

  g_atomic_int_set(&loop_live, 1);
  GST_INFO("Adding doStart to Bus.");
  g_idle_add((GSourceFunc)doStart, args);

  // Add doWork to the bus.  We don't really have a main function which is why
  // we need to do doWork here.  This is less efficient than
  // CefRunMessageLoop(), but we need the UI thread unblocked.
  while (g_atomic_int_get(&loop_live))
  {
    // Decreasing the doWork frequency is more efficient, but it causes us to
    // lose a bunch of frames if it gets too close to our target framerate.
    g_usleep(6000);
    g_idle_add((GSourceFunc)doWork, NULL);
  }
  GST_INFO("MessageLoop Ended");
}

void doOpenBrowser(gpointer args)
{
  GST_INFO("doOpenBrowser");
  g_mutex_lock(&cef_start_mutex);
  while (!app)
  {
    g_cond_wait(&cef_start_cond, &cef_start_mutex);
  }
  g_mutex_unlock(&cef_start_mutex);
  g_idle_add((GSourceFunc)doOpen, args);
}

void open_browser(gpointer args)
{
  g_thread_new("open_browser", (GThreadFunc)doOpenBrowser, args);
}

static bool doSetSize(void *args)
{
  gstSizeArgs *sizeArgs = (gstSizeArgs *)args;
  int width = sizeArgs->width;
  int height = sizeArgs->height;
  void *gstCef = sizeArgs->gstCef;
  g_free(args);
  GST_INFO("set size to %u by %u", width, height);

  if (!app)
  {
    return false;
  }

  app->SetSize(gstCef, width, height);
  return false;
}

void set_size(void *args)
{
  GST_INFO("set_size");
  g_idle_add((GSourceFunc)doSetSize, args);
}

static bool doSetHidden(void *args)
{
  GST_DEBUG("doSetHidden");
  gstHiddenArgs *hiddenArgs = (gstHiddenArgs *)args;
  bool hidden = hiddenArgs->hidden;
  void *gstCef = hiddenArgs->gstCef;
  g_free(args);

  if (!app)
  {
    return false;
  }

  app->SetHidden(gstCef, hidden);
  return false;
}

static bool doExecuteJS(void *args)
{
  GST_DEBUG("doExecuteJS");
  struct gstExecuteJSArgs *exec_js_args = (gstExecuteJSArgs* )args;
  void *gstCef = exec_js_args->gstCef;
  CefString js;
  js.FromASCII(exec_js_args->js);
  g_free(exec_js_args->js);
  g_free(args);

  if (!app)
  {
    GST_INFO("Cannot execute JS because the app is not initialized.");
    return false;
  }

  app->ExecuteJS(gstCef, js);
  return false;
}

static bool doSetInitializationJS(void *args)
{
  GST_DEBUG("doSetInitializationJS");
  struct gstExecuteJSArgs *exec_js_args = (gstExecuteJSArgs*)args;
  void *gstCef = exec_js_args->gstCef;
  CefString js;
  js.FromASCII(exec_js_args->js);
  g_free(exec_js_args->js);
  g_free(args);

  if (!app)
  {
    GST_INFO("Cannot set initialization JS because the app is not initialized.");
    return false;
  }

  app->SetInitializationJS(gstCef, js);
  return false;
}

void set_hidden(void *args)
{
  GST_DEBUG("Adding set_hidden to work loop.");
  g_idle_add((GSourceFunc)doSetHidden, args);
}

void execute_js(void *args)
{
  GST_DEBUG("Adding doExecuteJS to work loop");
  g_idle_add((GSourceFunc)doExecuteJS, args);
}

void refresh_browser(gpointer *cef)
{
  if (app)
  {
    app.get()->Refresh(cef);
  }
  else
  {
    GST_ERROR("refresh_browser error: no app");
  }
  return;
}

void set_initialization_js(void *args)
{
  GST_DEBUG("Adding doExecuteJS to work loop");
  g_idle_add((GSourceFunc)doSetInitializationJS, args);
}

bool doClose(gpointer args)
{
  GST_DEBUG("doClose");
  if (app)
  {
    app->CloseBrowser(args, true, 1);
  }
  else
  {
    GST_ERROR("ERROR: no app");
  }
  return false;
}

void stop_rendering(gpointer *cef)
{
  if (app)
  { 
    app.get()->SetHidden(cef, TRUE);
  }
  else
  {
    GST_ERROR("ERROR: no app");
  }
  return;
}

void start_rendering(gpointer *cef)
{
  if (app)
  {
    app.get()->SetHidden(cef, FALSE);
  }
  else
  {
    GST_ERROR("ERROR: no app");
  }
  return;
}


void close_browser(gpointer args)
{
  g_idle_add((GSourceFunc)doClose, args);
}

void shutdown_browser()
{
  GST_WARNING("shutdown browser");
  g_atomic_int_set(&loop_live, 0);
  g_usleep(2 * G_USEC_PER_SEC);
  g_idle_add((GSourceFunc)doShutdown, NULL);
}
