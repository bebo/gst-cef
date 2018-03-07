// Copyright (c) 2017 Bebo
// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cef_handler.h"
#include "cef.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#include <windows.h>

#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

BrowserClient::BrowserClient(CefString url, int width, int height,
CefString initialization_js, void *push_frame, void *gst_cef) :
is_closing_(false),
url_(url),
width_(width),
height_(height),
initialization_js_(initialization_js),
push_frame_(push_frame),
gst_cef_(gst_cef) {}

BrowserClient::~BrowserClient() {}

bool BrowserClient::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
  GST_DEBUG("rect i got cef: %uX%u", cef->width, cef->height);
  rect.Set(0, 0, width_, height_);
  return true;
}

void BrowserClient::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType paintType, const RectList &rects,
                            const void *buffer, int width, int height)
{
  GST_DEBUG("OnPaint");
  if (!ready_)
  {
    GST_DEBUG("Not ready for OnPaint yet");
    return;
  }
  // TODO: Make sure that the settings and gstreamer lineup.
  push_frame(gst_cef_, buffer, width_, height_);
}

void BrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
  // The first callback to reference browser.
  browser_ = browser;
  GST_DEBUG("OnAfterCreated: %d", browser->GetIdentifier());
}

void BrowserClient::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
  // Called after DoClose.
  is_closing_ = true;
  GST_DEBUG("Cef OnBeforeClose");
}

void BrowserClient::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString &errorText,
                                const CefString &failedUrl)
{
  CEF_REQUIRE_UI_THREAD();
  GST_ERROR("OnLoadError");

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

void BrowserClient::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                         bool isLoading,
                                         bool canGoBack,
                                         bool canGoForward)
{
  CEF_REQUIRE_UI_THREAD();
  GST_INFO("OnLoadingStateChange: %d", isLoading);
}

void BrowserClient::Refresh(CefRefPtr<CefBrowser> browser,
                            CefRefPtr<CefFrame> frame)
{
  GST_INFO("Refresh - window id: %d", browser->GetIdentifier());
  browser->Reload();
}

void BrowserClient::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode)
{
  if (httpStatusCode >= 200 && httpStatusCode < 400)
  {
    GST_INFO("OnLoadEnd - window id: %d, is main: %d, status code: %d",
             browser->GetIdentifier(), frame->IsMain(), httpStatusCode);
  }
  else if (httpStatusCode)
  {
    GST_ERROR("OnLoadEnd - window id: %d, is main: %d, status code: %d",
              browser->GetIdentifier(), frame->IsMain(), httpStatusCode);
  }

  if (frame->IsMain())
  {
    if (httpStatusCode == 200)
    { // 200 ~ 499?
      GST_INFO("Cef Initialization JavaScript: %s", initialization_js_);
      frame->ExecuteJavaScript(CefString(initialization_js), frame->GetURL(), 0);
      GST_INFO("Executed startup javascript.");
      retry_count = 0;
      ready = true;
    }
    else if (httpStatusCode >= 500 && httpStatusCode < 600)
    { // 500 ~ 599?
      const char *url = frame->GetURL().ToString().c_str();

      unsigned long long retry_time_ms = (std::min)((unsigned long long)30000, (unsigned long long)std::pow(2, retry_count) * 200); // 200, 400, 800, 1600 ... 30000

      GST_INFO("OnLoadEnd - scheduled a refresh. window id: %d, status code: %d, url: %s - refreshing in %llums, count: %d",
               browser->GetIdentifier(), httpStatusCode, url, retry_time_ms, retry_count);

      CefPostDelayedTask(TID_UI, base::Bind(&BrowserClient::Refresh, this, browser, frame), retry_time_ms);
      retry_count++;
    }
  }
}

void BrowserClient::CloseBrowser(void *gst_cef, bool force_close)
{
  GST_DEBUG("CloseBrowser. force_close: %d", force_close);
  browser_->GetHost()->CloseBrowser(force_close);
}

void BrowserClient::SetHidden(bool hidden)
{
  hidden_ = hidden;
  CefRefPtr<CefBrowserHost> host = browser_->GetHost();
  host->WasHidden(hidden);
}

void BrowserClient::ExecuteJS(CefString js)
{
  GST_DEBUG("BrowserClient::ExecuteJS %s", js);
  CefRefPtr<CefFrame> frame = browser_->GetMainFrame();
  frame->ExecuteJavaScript(js, frame->GetURL(), 0);
}

void BrowserClient::SetSize(int width, int height)
{
  GST_INFO("BrowserClient::Setting size of browser to %ux%u", width, height);
  width_ = width;
  height_ = height;
  CefBrowserHost *host = browser_->GetHost().get();
  host->WasResized();
}
