// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_HANDLER_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_HANDLER_H_

#include "include/cef_client.h"
#include <list>
#include <map>

typedef struct GstCefInfo
{
  void *gst_cef;
  void (*push_frame)(void *gstCef, const void *buffer, int width, int height);
  CefRefPtr<CefBrowser> browser;

  int width;
  int height;
  int retry_count;
  bool ready;
  char *initialization_js;
  struct timeval last_tv;
} GstCefInfo_T;

class BrowserClient : public CefClient,
                      public CefDisplayHandler,
                      public CefLifeSpanHandler,
                      public CefLoadHandler,
                      public CefRenderHandler
{
public:
  explicit BrowserClient();
  ~BrowserClient();

  // Provide access to the single global instance of this object.
  /* static BrowserClient* GetInstance(); */

  // CefClient methods:
  virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE { return this; }
  virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE { return this; }
  virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE { return this; }
  virtual CefRefPtr<CefRenderHandler> GetRenderHandler() OVERRIDE { return this; }

  // CefDisplayHandler methods:
  virtual void OnTitleChange(CefRefPtr<CefBrowser> browser,
                             const CefString &title) OVERRIDE;

  // CefLifeSpanHandler methods:
  virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
  virtual bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
  virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

  // CefLoadHandler methods:
  virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           ErrorCode errorCode,
                           const CefString &errorText,
                           const CefString &failedUrl) OVERRIDE;

  virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                    bool isLoading,
                                    bool canGoBack,
                                    bool canGoForward) OVERRIDE;

  virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         int httpStatusCode) OVERRIDE;

  bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect);
  void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType paintType,
               const RectList &rects, const void *buffer, int width, int height) OVERRIDE;

  // Request that all existing browser windows close.
  void CloseBrowser(void *gst_cef, bool force_close);
  void SetSize(void *gst_cef, int width, int height);
  void SetHidden(void *gst_cef, bool hidden);
  void ExecuteJS(void *gst_cef, char* js);
  void AddBrowserGstMap(CefRefPtr<CefBrowser> browser, void *gstCef, void *push_frame, int width, int height, char *initialization_js);

  bool IsClosing() const { return is_closing_; }

private:
  // Platform-specific implementation.
  void PlatformTitleChange(CefRefPtr<CefBrowser> browser,
                           const CefString &title);

  // True if the application is using the Views framework.
  const bool use_views_;

  bool is_closing_;

  // List of existing browser windows. Only accessed on the CEF UI thread.
  std::map<int, GstCefInfo_T *> browser_gst_map;

  GstCefInfo_T *getGstCef(CefRefPtr<CefBrowser> browser);

  void Refresh(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame);

  // Include the default reference counting implementation.
  IMPLEMENT_REFCOUNTING(BrowserClient);
};

#endif // CEF_TESTS_CEFSIMPLE_SIMPLE_HANDLER_H_
