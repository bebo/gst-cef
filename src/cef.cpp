

#include <ctime>
#include <iostream>
#include <list>
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

}  // namespace

#if 0
RenderHandler::RenderHandler(int width, int height) {
    this->width = width;
    this->height = height;
  }

bool RenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    std::cout << "GetViewRect" << std::endl;
    rect = CefRect(0, 0, width, height);
    return true;
}

void RenderHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType paintType, const RectList &rects, 
               const void *buffer, int width, int height) {
    std::cout << "OnPaint() for size: " << width << " x " << height << std::endl;
    /* memcpy(frame_buffer, buffer, width * height * 4 * 1); */
  }

/* class BrowserClient : public CefClient, public CefLifespanHandler { */
BrowserClient::BrowserClient(RenderHandler* renderHandler) :
     renderHandler(renderHandler)
{
}


BrowserClient::~BrowserClient() {
}

CefRefPtr<CefRenderHandler> BrowserClient::GetRenderHandler() {
    return renderHandler;
}


Browser::~Browser() {};

bool Browser::init(const char * url, uint32_t width, uint32_t height, uint32_t fps) {
    std::cout << "Browser::init" << std::endl;
    /* CefMainArgs args(GetModuleHandle(NULL)); */
    XSetErrorHandler(XErrorHandlerImpl);
    XSetIOErrorHandler(XIOErrorHandlerImpl);

    CefMainArgs* cefMainArgs = new CefMainArgs(0, nullptr);
    CefSettings settings;
    /* settings.windowless_rendering_enabled = true; */
    // Should not be required:
    settings.no_sandbox = false;
    settings.multi_threaded_message_loop = false;
    CefString(&settings.browser_subprocess_path).FromASCII("/home/fpn/gst-cef/src/subprocess");

    /* CEF_REQUIRE_UI_THREAD(); */
    /* if (CefInitialize(*cefMainArgs, settings, this, NULL)) */
    std::cout << "CefInitialize" << std::endl;
    if (CefInitialize(*cefMainArgs, settings, NULL, NULL))
    {
        std::cout << "Browser initialized " << url << std::endl;
        delete cefMainArgs;

        render_handler_ = new RenderHandler(width, height);

        browser_client_ = new BrowserClient(render_handler_);

        CefWindowInfo window_info;
        window_info.SetAsWindowless(0, true);

        CefBrowserSettings browser_settings;
        browser_settings.windowless_frame_rate = fps;

        CefString url = url;
        /* browser_ = CefBrowserHost::CreateBrowserSync(window_info, browser_client_.get(), url, browser_settings, nullptr); */
        browser_ = CefBrowserHost::CreateBrowserSync(window_info, NULL, url, browser_settings, nullptr);

        return true;
    }

    std::cout << "Unable to initialize" << std::endl;
    return false;
}

void Browser::OnBeforeCommandLineProcessing(
    const CefString& process_type,
    CefRefPtr<CefCommandLine> command_line) {
  // Pass additional command-line flags to the browser process.
  if (process_type.empty()) {
    // Pass additional command-line flags when off-screen rendering is enabled.
    /* if (command_line->HasSwitch(switches::kOffScreenRenderingEnabled)) { */
      // If the PDF extension is enabled then cc Surfaces must be disabled for
      // PDFs to render correctly.
      // See https://bitbucket.org/chromiumembedded/cef/issues/1689 for details.
      /* if (!command_line->HasSwitch("disable-extensions") && */
      /*     !command_line->HasSwitch("disable-pdf-extension")) { */
      /*   command_line->AppendSwitch("disable-surfaces"); */
      /* } */

      // Use software rendering and compositing (disable GPU) for increased FPS
      // and decreased CPU usage. This will also disable WebGL so remove these
      // switches if you need that capability.
      // See https://bitbucket.org/chromiumembedded/cef/issues/1257 for details.
      /* if (!command_line->HasSwitch(switches::kEnableGPU)) { */
        /* command_line->AppendSwitch("off-screen-rendering-enabled"); */
        command_line->AppendSwitch("disable-gpu");
        command_line->AppendSwitch("disable-gpu-compositing");
      /* } */
    /* } */

    /* if (command_line->HasSwitch(switches::kUseViews) && */
    /*     !command_line->HasSwitch("top-chrome-md")) { */
      // Use non-material mode on all platforms by default. Among other things
      // this causes menu buttons to show hover state. See usage of
      // MaterialDesignController::IsModeMaterial() in Chromium code.
      /* command_line->AppendSwitchWithValue("top-chrome-md", "non-material"); */
    /* } */

    /* DelegateSet::iterator it = delegates_.begin(); */
    /* for (; it != delegates_.end(); ++it) */
    /*   (*it)->OnBeforeCommandLineProcessing(this, command_line); */
  }
}

/*         void update() */
/*         { */
/*             CefDoMessageLoopWork(); */
/*         } */

/*         /1* void requestExit() *1/ */
/*         /1* { *1/ */
/*         /1*     if (browser_.get() && browser_->GetHost()) *1/ */
/*         /1*     { *1/ */
/*         /1*         browser_->GetHost()->CloseBrowser(true); *1/ */
/*         /1*     } *1/ */
/*         /1* } *1/ */

void Browser::shutdown()
{
    render_handler_ = NULL;
    browser_client_ = NULL;
    browser_ = NULL;
    CefShutdown();
};



void new_browser(void ** browser, const char * url, uint32_t width, uint32_t
        height, uint32_t fps, void * new_frame_callback) {
    std::cout << "new browser" << std::endl;
    CefRefPtr<Browser> b = new Browser();
    b->init(url, width, height, fps);
    browser = (void **) b.get();
}


void browser_loop(void * args) {
  void * browser = NULL;
  new_browser(&browser, "https://bebo.com", 1280, 720, 30, NULL);
  std::cout << "starting browser_loop" << std::endl;

  // Run the CEF message loop. This will block until CefQuitMessageLoop() is called.
  //false
  while (1) {
      /* CefDoMessageLoopWork(); */
      std::cout << "running loop" << std::endl;
      usleep(100000);
  }
  /* CefRunMessageLoop(); */
  std::cout << "ended browser_loop" << std::endl;

    /* CefRefPtr<Browser> *b = new CefRefPtr<Browser>((Browser*) *browser); */
    /* /1* &b = new (CefRefPtr<Browser>) browser; *1/ */

    /* time_t start_time; */
    /* time(&start_time); */

    /* MSG msg; */
    /* do */
    /* { */
    /*     if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) */
    /*     { */
    /*         TranslateMessage(&msg); */
    /*         DispatchMessage(&msg); */
    /*     } */
    /*     else */
    /*     { */
    /*         b->update(); */

    /*         if (time(NULL) > start_time + 3) */
    /*         { */
    /*             b->requestExit(); */
    /*         } */

    /*     } */
    /* } */
    /* while (msg.message != WM_QUIT); */

    /* *b->shutdown(); */
    /* b = NULL; */
    /* *browser = NULL; */
}


void quit_browser() {
    CefQuitMessageLoop();
}
#endif

void browser_loop(void * args) {
    /* CefMainArgs* cefMainArgs = new CefMainArgs(0, nullptr); */
  // Provide CEF with command-line arguments.
  CefMainArgs main_args(0, nullptr);

  // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
  // that share the same executable. This function checks the command-line and,
  // if this is a sub-process, executes the appropriate logic.
  /* int exit_code = CefExecuteProcess(main_args, NULL, NULL); */
  /* if (exit_code >= 0) { */
  /*   // The sub-process has completed so return here. */
  /*   /1* return exit_code; *1/ */
  /*   return ; */
  /* } */

  // Install xlib error handlers so that the application won't be terminated
  // on non-fatal errors.
  XSetErrorHandler(XErrorHandlerImpl);
  XSetIOErrorHandler(XIOErrorHandlerImpl);

  // Specify CEF global settings here.
  CefSettings settings;
  CefString(&settings.browser_subprocess_path).FromASCII("/home/fpn/gst-cef/src/subprocess");

  // SimpleApp implements application-level callbacks for the browser process.
  // It will create the first browser instance in OnContextInitialized() after
  // CEF has initialized.
  CefRefPtr<SimpleApp> app(new SimpleApp);

  // Initialize CEF for the browser process.
  CefInitialize(main_args, settings, app.get(), NULL);

  // Run the CEF message loop. This will block until CefQuitMessageLoop() is
  // called.
  CefRunMessageLoop();

  // Shut down CEF.
  CefShutdown();

  /* return 0; */
}
