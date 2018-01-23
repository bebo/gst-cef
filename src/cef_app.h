// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_

#include "include/cef_app.h"
#include "cef_handler.h"

// Implement application-level callbacks for the browser process.
class Browser: public CefApp, public CefBrowserProcessHandler {
 public:
  Browser(void * gstCef, void * push_data, char* url, int width, int height);

  // CefApp methods:
  virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler()
      OVERRIDE {
    return this;
  }

  // CefBrowserProcessHandler methods:
  virtual void OnContextInitialized() OVERRIDE;
  void CloseBrowser(void * gst_cef, bool force_close);
  void Open(void *gstCef, void *push_data, char* url, int width, int height);
  void SetSize(void *gstCef, int width, int height);

 private: 
  CefRefPtr<BrowserClient> browserClient;
  void * gstCef;
  void * push_data;
  int height;
  int width;
  char * url;

  // Include the default reference counting implementation.
  IMPLEMENT_REFCOUNTING(Browser);
};

#endif  // CEF_TESTS_CEFSIMPLE_SIMPLE_APP_H_
