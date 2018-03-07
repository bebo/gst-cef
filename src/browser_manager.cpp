// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cef.h"
#include "cef_app.h"

#include <string>
#include <sstream>
#include <iostream>
#include <map>

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"
#include "cef_handler.h"

Browser::Browser() : initialized_(false) {};

void Browser::CloseBrowser(void *gst_cef, bool force_close)
{
  GST_LOG("Browser::CloseBrowser");
  // browserClient->CloseBrowser(gst_cef, force_close);
}

void Browser::Open(void *gst_cef, void *push_frame, CefString url, int width, int height, CefString initialization_js)
{
  // TODO: Make sure this method is called on the UI thread.
  GST_INFO("Open new browser window: %s", open_url);
  CefRefPtr<CefWindowManager> client = new CefWindowManager(url, width, height, initialization_js, push_frame, gst_cef);
  browsers_.push_back(client);
  if (initialized_)
    CreateWindow(client);
}

void Browser::CreateWindow(CefRefPtr<CefWindowManager> client) {
  CefWindowInfo window_info;
  window_info.width = width;
  window_info.height = height;
  window_info.SetAsWindowless(0);

  CefBrowserSettings browser_settings;
  browser_settings.windowless_frame_rate = 30;

  CefRefPtr<CefBrowser> browser = CefBrowserHost::CreateBrowserSync(window_info, client, url, browser_settings, NULL);
  browser->GetHost()->WasResized();
}

void Browser::SetSize(void *gst_cef, int width, int height)
{
  CEF_REQUIRE_UI_THREAD();
  GST_INFO("Browser::SetSize");
  // browserClient->SetSize(gstCef, width, height);
}

void Browser::SetHidden(void *gst_cef, bool hidden)
{
  CEF_REQUIRE_UI_THREAD();
  // browserClient->SetHidden(gstCef, hidden);
}

void Browser::ExecuteJS(void *gst_cef, CefString js)
{
  CEF_REQUIRE_UI_THREAD();
  //browserClient->ExecuteJS(gstCef, js);
}

void Browser::OnContextInitialized()
{
  GST_INFO("OnContextInitialized");
  for(int i = 0; i < browsers_.size(); i++) {
    CreateWindow(browsers_[i]);
  }
  initialized_ = true;
}

void Browser::OnBeforeCommandLineProcessing(
    const CefString &process_type,
    CefRefPtr<CefCommandLine> command_line)
{
  command_line->AppendSwitch("disable-gpu");
  command_line->AppendSwitch("disable-gpu-compositing");
  command_line->AppendSwitch("enable-begin-frame-scheduling");
  command_line->AppendSwitch("enable-system-flash");
}
