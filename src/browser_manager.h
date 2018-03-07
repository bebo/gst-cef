// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_

#include "include/cef_app.h"
#include "cef_client.h"

// Implement application-level callbacks for the browser process.
// This is a singleton class and all functions in this class need to be called on the UI thread.
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
  void Open(void *gstCef, void *push_data, char *open_url, int width, int height, char *initialization_js);
  void SetSize(void *gstCef, int width, int height);
  void SetHidden(void *gstCef, bool hidden);
  void ExecuteJS(void *gstCef, char* js);

private:
  std:<BrowserClient> browsers_;
  int browser_id_;
  std::atomic<bool> initialized_;
  IMPLEMENT_REFCOUNTING(Browser);
};

#endif // CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_
