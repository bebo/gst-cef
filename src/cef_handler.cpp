// Copyright (c) 2017 Bebo
// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cef_handler.h"

#include <iostream>
#include <string>

#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include <sys/time.h>


RenderHandler::RenderHandler(void * gstCef, void * push_frame, int width, int height):
    ready(0)
{
    this->push_frame = (void(*)(void *gstCef, const void *buffer, int width, int height))push_frame;
    /* this->push_frame = void (*p)() =  (void(*)(void *gstCef, const void *buffer, int width, int height))push_frame; */
    this->gstCef = gstCef;
    this->width = width;
    this->height = height;
    gettimeofday(&this->last_tv, NULL);
  }

bool RenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    std::cout << "GetViewRect" << std::endl;
    rect = CefRect(0, 0, width, height);
    return true;
}

void RenderHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType paintType, const RectList &rects, 
               const void *buffer, int width, int height) {
    if (! this->ready) {
        std::cout << "Not ready yet" <<std::endl;
        return;
    }
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long millisecondsSinceEpoch = 
         ((unsigned long long)(tv.tv_sec) - (unsigned long long)(this->last_tv.tv_sec)) * 1000000 
         + (unsigned long long)(tv.tv_usec) - (unsigned long long)(this->last_tv.tv_usec);
    last_tv.tv_sec = tv.tv_sec;
    last_tv.tv_usec = tv.tv_usec;


    std::cout << "OnPaint() for size: " << width << " x " << height << " ms:" << millisecondsSinceEpoch <<std::endl;
    this->push_frame(this->gstCef, buffer, width, height);
}

BrowserClient::BrowserClient(RenderHandler* renderHandler)
    : use_views_(false), is_closing_(false), renderHandler(renderHandler) {
}

BrowserClient::~BrowserClient() {
}

CefRefPtr<CefRenderHandler> BrowserClient::GetRenderHandler() {
    return renderHandler;
}


void BrowserClient::OnTitleChange(CefRefPtr<CefBrowser> browser,
                                  const CefString& title) {
  CEF_REQUIRE_UI_THREAD();

  if (use_views_) {
    // Set the title of the window using the Views framework.
    CefRefPtr<CefBrowserView> browser_view =
        CefBrowserView::GetForBrowser(browser);
    if (browser_view) {
      CefRefPtr<CefWindow> window = browser_view->GetWindow();
      if (window)
        window->SetTitle(title);
    }
  } else {
    // Set the title of the window using platform APIs.
    PlatformTitleChange(browser, title);
  }
}

void BrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Add to the list of existing browsers.
  browser_list_.push_back(browser);
}

bool BrowserClient::DoClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  // Closing the main window requires special handling. See the DoClose()
  // documentation in the CEF header for a detailed destription of this
  // process.
  if (browser_list_.size() == 1) {
    // Set a flag to indicate that the window close should be allowed.
    is_closing_ = true;
  }

  // Allow the close. For windowed browsers this will result in the OS close
  // event being sent.
  return false;
}

void BrowserClient::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  std::cout << "OnBeforeClose" << std::endl;
  // Remove from the list of existing browsers.
  BrowserList::iterator bit = browser_list_.begin();
  for (; bit != browser_list_.end(); ++bit) {
    if ((*bit)->IsSame(browser)) {
      browser_list_.erase(bit);
      break;
    }
  }

  if (browser_list_.empty()) {
    // All browser windows have closed. Quit the application message loop.
    //CefQuitMessageLoop();
  }
}

void BrowserClient::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) {
  CEF_REQUIRE_UI_THREAD();

  // Don't display an error for downloaded files.
  if (errorCode == ERR_ABORTED)
    return;

  // Display a load error message.
  std::stringstream ss;
  ss << "<html><body bgcolor=\"transparent\">"
        "<h2>Failed to load URL "
     << std::string(failedUrl) << " with error " << std::string(errorText)
     << " (" << errorCode << ").</h2></body></html>";
  frame->LoadString(ss.str(), failedUrl);
}
void BrowserClient::OnLoadingStateChange(CefRefPtr< CefBrowser > browser,
  bool isLoading,
  bool canGoBack,
  bool canGoForward) {
  CEF_REQUIRE_UI_THREAD();
  std::cout << "OnLoadingStateChange: " << isLoading << std::endl;
}

void BrowserClient::OnLoadEnd(CefRefPtr< CefBrowser > browser,
  CefRefPtr< CefFrame > frame,
  int httpStatusCode) {
  CEF_REQUIRE_UI_THREAD();
  std::cout << "OnLoadEnd - isMain: " << frame->IsMain() << " status code: " << httpStatusCode << std::endl;
  if (frame->IsMain() && httpStatusCode == 200) {
      this->renderHandler->SetReady(true);
  }
}

void BrowserClient::CloseAllBrowsers(bool force_close) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute on the UI thread.
    CefPostTask(TID_UI, base::Bind(&BrowserClient::CloseAllBrowsers, this,
                                   force_close));
    return;
  }

  if (browser_list_.empty())
    return;

  BrowserList::const_iterator it = browser_list_.begin();
  for (; it != browser_list_.end(); ++it)
    (*it)->GetHost()->CloseBrowser(force_close);
}
