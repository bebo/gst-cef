// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_HANDLER_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_HANDLER_H_

#include "include/cef_client.h"
#include <list>
#include <map>

class CefWindowManager : public CefClient,
                      public CefDisplayHandler,
                      public CefLifeSpanHandler,
                      public CefLoadHandler,
                      public CefRenderHandler
{
public:
  explicit CefWindowManager(CefString url, int width, int height, CefString initialization_js, void *push_frame_, void *gst_cef);
  ~CefWindowManager();

  // Provide access to the single global instance of this object.
  /* static CefWindowManager* GetInstance(); */

  // CefClient methods:
  virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() OVERRIDE { return this; }
  virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE { return this; }
  virtual CefRefPtr<CefLoadHandler> GetLoadHandler() OVERRIDE { return this; }
  virtual CefRefPtr<CefRenderHandler> GetRenderHandler() OVERRIDE { return this; }

  // CefLifeSpanHandler methods:
  virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
  virtual bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
  virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;

  // CefLoadHandler methods:
  // They will be called on the UI thread.
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
  void CloseBrowser(bool force_close);
  void SetSize(int width, int height);
  void SetHidden(bool hidden);
  void ExecuteJS(CefString js);
  void AddBrowserGstMap(CefRefPtr<CefBrowser> browser, void *gstCef, void *push_frame, int width, int height, char *initialization_js);

  bool IsClosing() const { return is_closing_; }
  int GetWidth();
  int GetHeight();
  CefString GetUrl();
private:
  CefRefPtr<CefBrowser> browser_;
  bool is_closing_;
  void (* push_frame)(void *gst_cef, const void *buffer, int width, int height);
  void *gst_cef_;

  bool ready_;
  bool hidden_;
  int width_;
  int height_;
  int retry_count_;
  CefString url_;
  std::string initialization_js_;

  void Refresh(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame);

  // Include the default reference counting implementation.
  IMPLEMENT_REFCOUNTING(CefWindowManager);
};

#endif // CEF_TESTS_CEFSIMPLE_SIMPLE_HANDLER_H_
