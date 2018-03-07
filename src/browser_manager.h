// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_

#include "include/cef_app.h"
#include "cef_window_manager.h"

// Implement application-level callbacks for the browser process.
// This is a singleton class and all public functions in this class need to be called on the UI thread.
// Most of the CEF callbacks are guaranteed to be called on the UI thread.
class Browser : public CefApp, public CefBrowserProcessHandler, public CefRenderProcessHandler
{
public:
  Browser();

  // CefApp methods:
  virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler()
      OVERRIDE
  {
    return this;
  }

  virtual void OnBeforeCommandLineProcessing(
      const CefString &process_type,
      CefRefPtr<CefCommandLine> command_line) OVERRIDE;

  // CefBrowserProcessHandler methods:
  virtual void OnContextInitialized() OVERRIDE;
  void CloseBrowser(void *gst_cef, bool force_close);
  void Open(void *gst_cef, void *push_frame, CefString url, int width, int height, CefString initialization_js);
  void SetSize(void *gst_cef, int width, int height);
  void SetHidden(void *gst_cef, bool hidden);
  void ExecuteJS(void *gst_cef, CefString js);
  void SetInitializationJS(void *gst_cef, CefString initialization_js);
  void CreateCefWindow(CefRefPtr<CefWindowManager> client);

private:
  CefRefPtr<CefWindowManager> GetClient(void* gst_cef);
  std::vector<CefRefPtr<CefWindowManager>> browsers_;
  int browser_id_;
  bool initialized_;
  IMPLEMENT_REFCOUNTING(Browser);
};

#endif // CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_
