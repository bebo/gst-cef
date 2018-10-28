// Copyright (c) 2017 Bebo
// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cef_window_manager.h"
#include "cef_gst_interface.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#ifdef __WIN
#include <windows.h>
#endif

#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

CefWindowManager::CefWindowManager(CefString url, int width, int height,
CefString initialization_js, void *push_frame, void *gst_cef) :
is_closing_(false),
ready_(false),
url_(url),
retry_count_(0),
width_(width),
height_(height),
initialization_js_(initialization_js),
gst_cef_(gst_cef) {
  this->push_frame = (void (*)(void *gst_cef, const void *buffer, int width, int height)) push_frame;
}

CefWindowManager::~CefWindowManager() {}

bool CefWindowManager::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
  //GST_LOG("GetViewRect: %uX%u", width_, height_);
  rect.Set(0, 0, width_, height_);
  return true;
}

void CefWindowManager::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType paintType, const RectList &rects,
  const void *buffer, int width, int height)
{
  if (!ready_) {
    GST_LOG("Not ready for OnPaint yet");
    return;
  }
  if (!gst_cef_) {
    GST_LOG("No gst cef instance for OnPaint %d, %d", width, height);
    return;
  } 

  // The rects are just a single rectangle as of October 2015.
  GST_DEBUG_OBJECT(gst_cef_, "OnPaint %d, %d", width, height);
  push_frame(gst_cef_, buffer, width, height);
  
}

void CefWindowManager::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
  // The first callback to reference browser.
  browser_ = browser;
  GST_DEBUG("OnAfterCreated: %d", browser->GetIdentifier());
}

void CefWindowManager::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
  // Called after DoClose.
  is_closing_ = true;
  // don't we have a reference to this???
  GST_DEBUG_OBJECT(gst_cef_, "OnBeforeClose");
  g_object_unref(gst_cef_);
  gst_cef_ = nullptr;
  browser_ = nullptr;
  ready_ = false;
  GST_DEBUG("Cef OnBeforeClose");
}

void CefWindowManager::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString &errorText,
                                const CefString &failedUrl)
{
  GST_WARNING("OnLoadError %S %S", failedUrl.c_str(), errorText.c_str());
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

void CefWindowManager::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                         bool isLoading,
                                         bool canGoBack,
                                         bool canGoForward)
{
  CEF_REQUIRE_UI_THREAD();
  GST_INFO("OnLoadingStateChange: %d", isLoading);
}

void CefWindowManager::OnLoadEnd(CefRefPtr<CefBrowser> browser,
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
      if (!initialization_js_.empty()) {
        GST_INFO("Cef Initialization JavaScript: %s", initialization_js_);
        frame->ExecuteJavaScript(initialization_js_, frame->GetURL(), 0);
        GST_INFO("Executed startup javascript.");
      }

      retry_count_ = 0;
      ready_ = true;
    }
    else if (httpStatusCode >= 500 && httpStatusCode < 600)
    { // 500 ~ 599?
      const char *url = frame->GetURL().ToString().c_str();

      unsigned long long retry_time_ms = (std::min)((unsigned long long)30000, (unsigned long long)std::pow(2, retry_count_) * 200); // 200, 400, 800, 1600 ... 30000

      GST_INFO("OnLoadEnd - scheduled a refresh. window id: %d, status code: %d, url: %s - refreshing in %llums, count: %d",
               browser->GetIdentifier(), httpStatusCode, url, retry_time_ms, retry_count_);

      CefPostDelayedTask(TID_UI, base::Bind(&CefWindowManager::Refresh, this), retry_time_ms);
      retry_count_++;
    }
  }
}

int CefWindowManager::GetWidth() {
  return width_;
}

int CefWindowManager::GetHeight() {
  return height_;
}

CefString CefWindowManager::GetUrl() {
  return url_;
}

void* CefWindowManager::GetGstCef() {
  return gst_cef_;
}

void CefWindowManager::CloseBrowser(bool force_close)
{
  GST_DEBUG("CloseBrowser. force_close: %d", force_close);
  ready_ = false;
  browser_->GetHost()->CloseBrowser(force_close);
}

CefRefPtr<CefBrowser> CefWindowManager::GetBrowser() {
  return browser_;
}

void CefWindowManager::SetHidden(bool hidden)
{
  if (browser_ == nullptr && !is_closing_) {
    CefPostDelayedTask(TID_UI, base::Bind(&CefWindowManager::SetHidden, this, hidden), 50);
    return;
  }
  hidden_ = hidden;
  CefRefPtr<CefBrowserHost> host = browser_->GetHost();
  host->WasHidden(hidden);
  if (!hidden) {
    host->Invalidate(PET_VIEW);
  }
}

void CefWindowManager::ExecuteJS(CefString js)
{
  if (browser_ == nullptr && !is_closing_) {
    GST_DEBUG("Browser null, calling ExecuteJS again in 50ms");
    CefPostDelayedTask(TID_UI, base::Bind(&CefWindowManager::ExecuteJS, this, js), 50);
    return;
  }
  GST_DEBUG("CefWindowManager::ExecuteJS %s", js);
  CefRefPtr<CefFrame> frame = browser_->GetMainFrame();
  frame->ExecuteJavaScript(js, frame->GetURL(), 0);
}

void CefWindowManager::SetInitializationJS(CefString js) {
  initialization_js_ = js;
}

void CefWindowManager::SetSize(int width, int height)
{
  if (browser_ == nullptr && !is_closing_) {
    GST_WARNING("Cannot set size, browser window is null");
    return;
  }
  GST_INFO("CefWindowManager::Setting size of browser to %ux%u", width, height);
  width_ = width;
  height_ = height;
  CefBrowserHost *host = browser_->GetHost().get();
  host->WasResized();
}

void CefWindowManager::Refresh()
{
  if (browser_ == nullptr && !is_closing_) {
    GST_WARNING("Cannot refresh, browser window is null");
    return;
  }
  GST_INFO("Reloading - window id: %d", browser_->GetIdentifier());
  browser_->ReloadIgnoreCache();
}
