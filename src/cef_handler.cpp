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

  // TODO: support changing url by gst_cef
BrowserClient::BrowserClient(): use_views_(false), is_closing_(false) {}

BrowserClient::~BrowserClient() {
  for (auto it = browser_gst_map.begin(); it != browser_gst_map.end(); ++it) {
    browser_gst_map.erase(it);
    delete it->second;
  }
  browser_gst_map.clear();
}

bool BrowserClient::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    std::cout << "GetViewRect: browser id: " << browser->GetIdentifier() << ", length: " << browser_gst_map.size() << std::endl;

    // TODO: getGstCef may returns NULL because GetViewRect is called before 
    // the CreateBrowserSync return. We add the browser to the map after 
    // CreateBrowserSync return. Should figure a better dynamic alternative
    // for the width and height
    auto cef = getGstCef(browser);

    if (cef == 0) {
      rect = CefRect(0, 0, 1280, 720); // FIXME: value should not be hardcoded
      return true;
    }

    rect = CefRect(0, 0, cef->width, cef->height);
    return true;
}

void BrowserClient::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType paintType, const RectList &rects, 
               const void *buffer, int width, int height) {
    std::cout << "OnPaint: browser id: " << browser->GetIdentifier() << ", length: " << browser_gst_map.size() << std::endl;

    auto cef = getGstCef(browser);
    if (! cef->ready) {
        std::cout << "Not ready yet" << std::endl;
        return;
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long millisecondsSinceEpoch = 
         ((unsigned long long)(tv.tv_sec) - (unsigned long long)(cef->last_tv.tv_sec)) * 1000000 
         + (unsigned long long)(tv.tv_usec) - (unsigned long long)(cef->last_tv.tv_usec);
    cef->last_tv.tv_sec = tv.tv_sec;
    cef->last_tv.tv_usec = tv.tv_usec;

    /* std::cout << "OnPaint() for size: " << width << " x " << height << " ms:" << millisecondsSinceEpoch <<std::endl; */
    cef->push_frame(cef->gst_cef, buffer, cef->width, cef->height);
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
  std::cout << "OnAfterCreated: " << browser->GetIdentifier() << std::endl;
}

void BrowserClient::AddBrowserGstMap(CefRefPtr<CefBrowser> browser, void * gstCef, void * push_frame, int width, int height) {
  CEF_REQUIRE_UI_THREAD();

  GstCefInfo_T * gst_cef_info = new GstCefInfo_T;
  gst_cef_info->gst_cef = gstCef;
  gst_cef_info->push_frame = (void(*)(void *gstCef, const void *buffer, int width, int height))push_frame;
  gst_cef_info->ready = 0;
  gst_cef_info->width = width;
  gst_cef_info->height = height;
  gst_cef_info->browser = browser;
  gettimeofday(&gst_cef_info->last_tv, NULL);

  int id = browser->GetIdentifier();
  std::cout << "Adding browser to gst map browser id: " << id << std::endl;

  browser_gst_map[id] = gst_cef_info;
}

GstCefInfo_T* BrowserClient::getGstCef(CefRefPtr<CefBrowser> browser) {
  CEF_REQUIRE_UI_THREAD();

  int id = browser->GetIdentifier();

  auto info_it = browser_gst_map.find(id);
  auto it_end = browser_gst_map.end();

  if (info_it == it_end) {
    std::cout << "Browser not found in the map, browser id: " << id << std::endl;
    return NULL;
  }

  return info_it->second;
}

bool BrowserClient::DoClose(CefRefPtr<CefBrowser> browser) {
  std::cout << "DoClose" << std::endl;

  CEF_REQUIRE_UI_THREAD();

  // Closing the main window requires special handling. See the DoClose()
  // documentation in the CEF header for a detailed destription of this
  // process.
  if (browser_gst_map.size() == 1) {
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

  // Remove from the browser from the map.
  for (auto it = browser_gst_map.begin(); it != browser_gst_map.end(); ++it) {
    if ((it->first) == browser->GetIdentifier()) {
      browser_gst_map.erase(it);
      break;
    }
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

  std::cout << "OnLoadEnd - window id: " << browser->GetIdentifier() << " isMain: " << frame->IsMain() << " status code: " << httpStatusCode << std::endl;

  auto cef = getGstCef(browser);

  if (frame->IsMain() && httpStatusCode == 200) {
      cef->ready = true;
  } 
  // TODO: retry, 30s max
}

void BrowserClient::CloseAllBrowsers(bool force_close) {
  if (!CefCurrentlyOn(TID_UI)) {
    // Execute on the UI thread.
    CefPostTask(TID_UI, base::Bind(&BrowserClient::CloseAllBrowsers, this,
                                   force_close));
    return;
  }

  for (auto it = browser_gst_map.begin(); it != browser_gst_map.end(); ++it) {
    CefRefPtr<CefBrowser> browser = it->second->browser;
    browser->GetHost()->CloseBrowser(force_close);
  }
}
