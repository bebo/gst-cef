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
#include "include/cef_app.h"
#include "include/wrapper/cef_closure_task.h"

#include "cef_gst_interface.h"
#include "browser_manager.h"
#include "cef_window_manager.h"

Browser::Browser() : initialized_(false) {};

void Browser::CloseBrowser(void *gst_cef, bool force_close, int count)
{
  GST_DEBUG("Browser::CloseBrowser");
  if (!CefCurrentlyOn(TID_UI) || count == 0) {
    GST_DEBUG("Need to close on UI thread. Adding to message loop");
    CefPostTask(TID_UI, base::Bind(&Browser::CloseBrowser, this, gst_cef, force_close, 1));
  }
  CefRefPtr<CefWindowManager> b;
  for (int i = 0; i < browsers_.size(); i++) {
    if (gst_cef == browsers_[i]->GetGstCef()) {
      b = browsers_[i];
      browsers_.erase(browsers_.begin() + i);
      GST_INFO("Closing browser at index: %d", i);
    }
  }
  if (b == nullptr) {
    return;
  }
  b->GetBrowser()->GetHost()->CloseBrowser(force_close);
}

CefRefPtr<CefWindowManager> Browser::GetClient(void* gst_cef) {
  if (!CefCurrentlyOn(TID_UI)) {
    GST_WARNING("Need to call Browser::GetClient on UI thread.");
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
  GST_DEBUG("Trying to open new window.");
  if (!CefCurrentlyOn(TID_UI) && initialized_) {
    // Before CefInitialize is called, the UI thread is not defined.  We still need to make sure that 
    // this is only called on the thread that will become the UI thread.
    GST_DEBUG("Need to create windows on the UI thread. Adding to message loop");
    CefPostTask(TID_UI, base::Bind(&Browser::Open, this, gst_cef, push_frame, url, width, height, initialization_js));
    return;
  }
  CefRefPtr<CefWindowManager> client = new CefWindowManager(url, width, height, initialization_js, push_frame, gst_cef);
  browsers_.push_back(client);
  GST_DEBUG("Added new window client to the browser list.");
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
  if (!CefCurrentlyOn(TID_UI)) {
    GST_INFO("Need to call Browser::SetSize on the UI thread. Adding to message loop");
    CefPostTask(TID_UI, base::Bind(&Browser::SetSize, this, gst_cef, width, height));
  }
  GST_INFO("Browser::SetSize");
  CefRefPtr<CefWindowManager> wm = GetClient(gst_cef);
  if (wm == nullptr) {
    GST_WARNING("Cannot set size.  Browser not found in map.");
    return;
  }
  wm->SetSize(width, height);
}

void Browser::SetHidden(void *gst_cef, bool hidden)
{
  if (!CefCurrentlyOn(TID_UI)) {
    GST_INFO("Need to call Browser::SetHidden on the UI thread. Adding to message loop");
    CefPostTask(TID_UI, base::Bind(&Browser::SetHidden, this, gst_cef, hidden));
  }
  GST_INFO("Browser::SetHidden");
  CefRefPtr<CefWindowManager> wm = GetClient(gst_cef);
  if (wm == nullptr) {
    GST_WARNING("Cannot set hidden.  Browser not found in map.");
    return;
  }
  wm->SetHidden(hidden);
}

void Browser::ExecuteJS(void *gst_cef, CefString js)
{
  if (!CefCurrentlyOn(TID_UI)) {
    GST_INFO("Need to call Browser::ExecuteJS on the UI thread. Adding to message loop");
    CefPostTask(TID_UI, base::Bind(&Browser::ExecuteJS, this, gst_cef, js));
  }
  GST_INFO("Browser::ExecuteJS");
  CefRefPtr<CefWindowManager> wm = GetClient(gst_cef);
  if (wm == nullptr) {
    GST_WARNING("Cannot execute JavaScript.  Browser not found in map.");
    return;
  }
  wm->ExecuteJS(js);
}

void Browser::SetInitializationJS(void *gst_cef, CefString js)
{
  if (!CefCurrentlyOn(TID_UI)) {
    GST_INFO("Need to call Browser::ExecuteJS on the UI thread. Adding to message loop");
    CefPostTask(TID_UI, base::Bind(&Browser::SetInitializationJS, this, gst_cef, js));
  }
  GST_INFO("Browser::ExecuteJS");
  CefRefPtr<CefWindowManager> wm = GetClient(gst_cef);
  if (wm == nullptr) {
    GST_DEBUG("Cannot set initialization JavaScript because the window does not exist yet.  It will be set later.");
    return;
  }
  wm->ExecuteJS(js);
}

void Browser::OnContextInitialized()
{
  GST_INFO("OnContextInitialized.  Creating %d windows", browsers_.size());
  for(int i = 0; i < browsers_.size(); i++) {
    CreateCefWindow(browsers_[i]);
  }
  initialized_ = true;
}

void Browser::OnBeforeCommandLineProcessing(
    const CefString &process_type,
    CefRefPtr<CefCommandLine> command_line)
{
  command_line->AppendSwitch("allow-file-access-from-files");
  command_line->AppendSwitch("disable-gpu");
  command_line->AppendSwitch("disable-gpu-compositing");
  command_line->AppendSwitch("enable-begin-frame-scheduling");
  command_line->AppendSwitch("enable-system-flash");
}
