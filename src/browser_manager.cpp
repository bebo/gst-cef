// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <string>
#include <sstream>
#include <iostream>
#include <map>

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"

#include "cef_gst_interface.h"
#include "browser_manager.h"
#include "cef_client.h"

Browser::Browser() : initialized_(false) {};

void Browser::CloseBrowser(void *gst_cef, bool force_close)
{
  GST_LOG("Browser::CloseBrowser");
  if (!CefCurrentlyOn(TID_UI)) {
    GST_INFO("Need to close on UI thread. Adding to message loop");
    // TODO: fix this.
    std::exit(1);
  }
  CefRefPtr<CefWindowManager> b;
  int index = -1;
  for (int i = 0; i < browsers_.size(); i++) {
    if (gst_cef == browsers_[i]->GetGstCef()) {
      index = i;
      b = browsers_[i];
      browsers_.erase(browsers_.begin() + i);
    }
  }
  if (index == -1) {
    GST_INFO("Failed to find browser when trying to close");
    return;
  }
  GST_INFO("Closing browser at index: %d", index);
  b->GetBrowser()->GetHost()->CloseBrowser(force_close);
}

CefRefPtr<CefWindowManager> Browser::GetClient(void* gst_cef) {
  // Must be called on UI thread.  
  if (!CefCurrentlyOn(TID_UI)) {
    GST_INFO("Need to close on UI thread. Adding to message loop");
    // TODO: fix this.
    std::exit(1);
  }

  GST_INFO("Getting browser client");
  for (int i = 0; i < browsers_.size(); i++) {
    if (gst_cef == browsers_[i]->GetGstCef()) {
      GST_INFO("found correct browser");
      return browsers_[i];
    }
  }
  GST_INFO("Did not found browser");
}

void Browser::Open(void *gst_cef, void *push_frame, CefString url, int width, int height, CefString initialization_js)
{
  // TODO: Make sure this method is called on the UI thread.
  GST_INFO("Open new browser window.");
  CefRefPtr<CefWindowManager> client = new CefWindowManager(url, width, height, initialization_js, push_frame, gst_cef);
  browsers_.push_back(client);
  if (initialized_) {
    CreateCefWindow(client);
  }
}

void Browser::CreateCefWindow(CefRefPtr<CefWindowManager> client) {
  CefWindowInfo window_info;
  window_info.width = client->GetWidth();
  window_info.height = client->GetHeight();
  window_info.SetAsWindowless(0);

  CefBrowserSettings browser_settings;
  browser_settings.windowless_frame_rate = 30;

  CefRefPtr<CefBrowser> browser = CefBrowserHost::CreateBrowserSync(window_info, client, client->GetUrl(), browser_settings, NULL);
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
    CreateCefWindow(browsers_[i]);
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
