// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cef.h"
#include "cef_app.h"

#include <string>
#include <sstream>
#include <iostream>

#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"
#include "cef_handler.h"

namespace {

// When using the Views framework this object provides the delegate
// implementation for the CefWindow that hosts the Views-based browser.
class BrowserWindowDelegate : public CefWindowDelegate {
 public:
  explicit BrowserWindowDelegate(CefRefPtr<CefBrowserView> browser_view)
      : browser_view_(browser_view) {}

  void OnWindowCreated(CefRefPtr<CefWindow> window) OVERRIDE {
    // Add the browser view and show the window.
    window->AddChildView(browser_view_);
    window->Show();

    // Give keyboard focus to the browser view.
    browser_view_->RequestFocus();
  }

  void OnWindowDestroyed(CefRefPtr<CefWindow> window) OVERRIDE {
    browser_view_ = NULL;
  }

  bool CanClose(CefRefPtr<CefWindow> window) OVERRIDE {
    // Allow the window to close if the browser says it's OK.
    CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
    if (browser)
      return browser->GetHost()->TryCloseBrowser();
    return true;
  }

 private:
  CefRefPtr<CefBrowserView> browser_view_;

  IMPLEMENT_REFCOUNTING(BrowserWindowDelegate);
  DISALLOW_COPY_AND_ASSIGN(BrowserWindowDelegate);
};

}  // namespace

Browser::Browser(void *gstCef, void *push_data, char* url, int width, int height): 
  gstCef(gstCef), push_data(push_data), url(url), width(width), height(height) {};

void Browser::CloseBrowser(void * gst_cef, bool force_close) {
  CEF_REQUIRE_UI_THREAD();

  GST_LOG("Browser::CloseBrowser");
  browserClient->CloseBrowser(gst_cef, force_close);
}

void Browser::Open(void *gstCef, void *push_data, char* url, int width, int height) {
  CEF_REQUIRE_UI_THREAD();
  GST_INFO("Open Url: %s", url);

  // CEF Window Settings
  CefWindowInfo window_info;
  window_info.width = width;
  window_info.height = height;
  // window_info.SetAsWindowless(0);

  // CEF Browser Settings
  CefBrowserSettings browser_settings;
  browser_settings.windowless_frame_rate = 30;

  CefRefPtr<CefBrowser> browser = CefBrowserHost::CreateBrowserSync(window_info, browserClient, url, browser_settings, NULL);
  GST_DEBUG("Synchronously created the browser.");
  browserClient->AddBrowserGstMap(browser, gstCef, push_data, width, height);
  GST_DEBUG("Added browser to gst map.");
  // browser->GetHost()->WasResized();
  // GST_DEBUG("Browser was resized.");
}

void Browser::SetSize(void *gstCef, int width, int height) {
  CEF_REQUIRE_UI_THREAD();
  std::cout << __func__ << std::endl;
  GST_INFO("Browser::SetSize");
  this->width = width;
  this->height = height;
  browserClient->SetSize(gstCef, width, height);
}

void Browser::SetHidden(void *gstCef, bool hidden) {
  CEF_REQUIRE_UI_THREAD();
  std::cout << __func__ << std::endl;
  browserClient->SetHidden(gstCef, hidden);
}

void Browser::OnContextInitialized() {
  CEF_REQUIRE_UI_THREAD();
  GST_INFO("OnContextInitialized");

  // BrowserClient implements browser-level callbacks.
  browserClient = new BrowserClient();
  Open(gstCef, push_data, url, this->width, this->height);
}

void Browser::OnBeforeCommandLineProcessing(
	const CefString& process_type,
	CefRefPtr<CefCommandLine> command_line) {
	//command_line->AppendSwitch("disable-gpu");
	//command_line->AppendSwitch("disable-gpu-compositing");
	//command_line->AppendSwitch("enable-begin-frame-scheduling");
	// command_line->AppendSwitch("enable-system-flash");
}
