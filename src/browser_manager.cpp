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

Browser::Browser() : browser_id_(0) {};

void Browser::CloseBrowser(void *gst_cef, bool force_close)
{
  CEF_REQUIRE_UI_THREAD();

  GST_LOG("Browser::CloseBrowser");
  browserClient->CloseBrowser(gst_cef, force_close);
}

void Browser::Open(void *gst_cef, void *push_data, std::string url, int width, int height, std::string initialization_js)
{
  // TODO: Make sure this method is called on the UI thread.
  GST_INFO("Open new browser window: %s", open_url);
  CefRefPtr<BrowserClient> client = new BrowserClient(url, width, height, initialization_js, push_frame, gst_cef);
  browsers_.push_back(client)

  // Enabling windowless rendering causes Chrome to fail to get the view rect and hit a NOTREACHED();
  // Doing a release build fixes this issue.
  // https://bitbucket.org/chromiumembedded/cef/src/fef43e07d688cb90381f7571f25b7912bded2b6e/libcef/browser/osr/render_widget_host_view_osr.cc?at=3112&fileviewer=file-view-default#render_widget_host_view_osr.cc-1120
  CefWindowInfo window_info;
  window_info.width = width;
  window_info.height = height;
  window_info.SetAsWindowless(0);

  CefBrowserSettings browser_settings;
  browser_settings.windowless_frame_rate = 30;

  CefRefPtr<CefBrowser> browser = CefBrowserHost::CreateBrowserSync(window_info, client, url, browser_settings, NULL);
  browser->GetHost()->WasResized();
  // TODO: I probably need to unref this somewhere later.
  client.SetBrowser(browser);
}

void Browser::SetSize(void *gstCef, int width, int height)
{
  CEF_REQUIRE_UI_THREAD();
  GST_INFO("Browser::SetSize");
  this->width = width;
  this->height = height;
  browserClient->SetSize(gstCef, width, height);
}

void Browser::SetHidden(void *gstCef, bool hidden)
{
  CEF_REQUIRE_UI_THREAD();
  browserClient->SetHidden(gstCef, hidden);
}

void Browser::ExecuteJS(void *gstCef, char *js)
{
  CEF_REQUIRE_UI_THREAD();
  browserClient->ExecuteJS(gstCef, js);
}

void Browser::OnContextInitialized()
{
  CEF_REQUIRE_UI_THREAD();
  GST_INFO("OnContextInitialized");

  // BrowserClient implements browser-level callbacks.
  browserClient = new BrowserClient();
  Open(gstCef, push_data, url, this->width, this->height, this->initialization_js);
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
