// Copyright (c) 2017 Bebo
// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cef_handler.h"
#include "cef.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

#include <windows.h>

#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

BrowserClient::BrowserClient(std::string url, int width, int height,
std::string initialization_js, void *push_frame, void *gst_cef) :
is_closing_(false),
url_(url),
width_(width),
height_(height),
initialization_js_(initialization_js),
push_frame_(push_frame),
gst_cef_(gst_cef) {}

BrowserClient::~BrowserClient() {}

bool BrowserClient::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
  GST_DEBUG("rect i got cef: %uX%u", cef->width, cef->height);
  rect = CefRect(0, 0, width_, height_);
  return true;
}

void BrowserClient::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType paintType, const RectList &rects,
                            const void *buffer, int width, int height)
{
  GST_DEBUG("OnPaint");
  if (!ready_)
  {
    GST_DEBUG("Not ready for OnPaint yet");
    return;
  }
  // TODO: Make sure that the settings and gstreamer lineup.
  push_frame(gst_cef_, buffer, width_, height_);
}

void BrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();
  GST_DEBUG("OnAfterCreated: %d", browser->GetIdentifier());
}

void BrowserClient::SetBrowser(CefRefPtr<CefBrowser> browser) {
    browser_ = browser;
}

void BrowserClient::AddBrowserGstMap(CefRefPtr<CefBrowser> browser, void *gstCef, void *push_frame, int width, int height, char* initialization_js)
{
  CEF_REQUIRE_UI_THREAD();

  GstCefInfo_T *gst_cef_info = new GstCefInfo_T;
  gst_cef_info->gst_cef = gstCef;
  gst_cef_info->push_frame = (void (*)(void *gstCef, const void *buffer, int width, int height))push_frame;
  gst_cef_info->ready = 0;
  gst_cef_info->retry_count = 0;
  gst_cef_info->width = width;
  gst_cef_info->height = height;
  gst_cef_info->browser = browser;
  gst_cef_info->initialization_js = initialization_js;

  int id = browser->GetIdentifier();
  GST_DEBUG("Adding browser to gst map browser id: %d", id);

  browser_gst_map[id] = gst_cef_info;
  //Do this because the caps may have triggered a SetSize before this function was called
  //this->SetSize(gstCef, width, height);
}

GstCefInfo_T *BrowserClient::getGstCef(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();

  int id = browser->GetIdentifier();

  auto info_it = browser_gst_map.find(id);
  auto it_end = browser_gst_map.end();

  if (info_it == it_end)
  {
    GST_DEBUG("Browser not found in the map, browser id: %d", id);
    return NULL;
  }

  return info_it->second;
}

bool BrowserClient::DoClose(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();

  GST_DEBUG("DoClose, browser id: %d", browser->GetIdentifier());

  // Closing the main window requires special handling. See the DoClose()
  // documentation in the CEF header for a detailed destription of this
  // process.
  if (browser_gst_map.size() == 1)
  {
    // Set a flag to indicate that the window close should be allowed.
    is_closing_ = true;
  }

  // Allow the close. For windowed browsers this will result in the OS close
  // event being sent.
  return false;
}

void BrowserClient::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
  CEF_REQUIRE_UI_THREAD();

  GST_INFO("OnBeforeClose, browser id: %d", browser->GetIdentifier());

  // Remove from the browser from the map.
  GstCefInfo_T *gstcefinfo = browser_gst_map[browser->GetIdentifier()];
  browser_gst_map.erase(browser->GetIdentifier());
  delete gstcefinfo;
}

void BrowserClient::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString &errorText,
                                const CefString &failedUrl)
{
  CEF_REQUIRE_UI_THREAD();
  GST_ERROR("OnLoadError");

  // Don't display an error for downloaded files.
  if (errorCode == ERR_ABORTED)
    return;

  // Display a load error message.
  std::stringstream ss;
  ss << "<html><body bgcolor=\"transparent\">"
        "<h2>Failed to load URL "
     << std::string(failedUrl) << " with error " << std::string(errorText)
     << " (" << errorCode << ").</h2></body></html>";
  frame->LoadString(ss.str(), failedUrl);
}

void BrowserClient::OnLoadingStateChange(CefRefPtr<CefBrowser> browser,
                                         bool isLoading,
                                         bool canGoBack,
                                         bool canGoForward)
{
  CEF_REQUIRE_UI_THREAD();
  GST_INFO("OnLoadingStateChange: %d", isLoading);
}

void BrowserClient::Refresh(CefRefPtr<CefBrowser> browser,
                            CefRefPtr<CefFrame> frame)
{
  GST_INFO("Refresh - window id: %d", browser->GetIdentifier());
  browser->Reload();
}

void BrowserClient::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode)
{
  if (httpStatusCode >= 200 && httpStatusCode < 400)
  {
    GST_INFO("OnLoadEnd - window id: %d, is main: %d, status code: %d",
             browser->GetIdentifier(), frame->IsMain(), httpStatusCode);
  }
  else if (httpStatusCode)
  {
    GST_ERROR("OnLoadEnd - window id: %d, is main: %d, status code: %d",
              browser->GetIdentifier(), frame->IsMain(), httpStatusCode);
  }

  if (frame->IsMain())
  {
    if (httpStatusCode == 200)
    { // 200 ~ 499?
      GST_INFO("Cef Initialization JavaScript: %s", initialization_js);
      frame->ExecuteJavaScript(CefString(initialization_js), frame->GetURL(), 0);
      GST_INFO("Executed startup javascript.");
      retry_count = 0;
      ready = true;
    }
    else if (httpStatusCode >= 500 && httpStatusCode < 600)
    { // 500 ~ 599?
      const char *url = frame->GetURL().ToString().c_str();

      unsigned long long retry_time_ms = (std::min)((unsigned long long)30000, (unsigned long long)std::pow(2, retry_count) * 200); // 200, 400, 800, 1600 ... 30000

      GST_INFO("OnLoadEnd - scheduled a refresh. window id: %d, status code: %d, url: %s - refreshing in %llums, count: %d",
               browser->GetIdentifier(), httpStatusCode, url, retry_time_ms, retry_count);

      CefPostDelayedTask(TID_UI, base::Bind(&BrowserClient::Refresh, this, browser, frame), retry_time_ms);
      retry_count++;
    }
  }
}

void BrowserClient::CloseBrowser(void *gst_cef, bool force_close)
{
  GST_DEBUG("CloseBrowser. force_close: %d", force_close);
  if (!CefCurrentlyOn(TID_UI))
  {
    // Execute on the UI thread.
    CefPostTask(TID_UI, base::Bind(&BrowserClient::CloseBrowser, this,
                                   gst_cef, force_close));
    return;
  }
  CEF_REQUIRE_UI_THREAD();

  for (auto it = browser_gst_map.begin(); it != browser_gst_map.end(); ++it)
  {
    if (!it->second)
    {
      GST_ERROR("Closing Browser, browser id: %d. UNABLE TO FIND it->second", it->first);
      continue;
    }

    if (it->second->gst_cef == gst_cef)
    {
      GST_LOG("Closing Browser, browser id: %d", it->first);
      CefBrowser *browser = it->second->browser.get();
      browser->GetHost()->CloseBrowser(force_close);
      break;
    }
  }
}

void BrowserClient::SetHidden(void *gst_cef, bool hidden)
{
  GST_INFO("Set Hidden");

  if (!CefCurrentlyOn(TID_UI))
  {
    // TODO: Determine if this works and is necessary.
    // Execute on the UI thread.
    CefPostTask(TID_UI, base::Bind(&BrowserClient::SetHidden, this,
                                   gst_cef, hidden));
    return;
  }
  CEF_REQUIRE_UI_THREAD();

  GST_INFO("BrowserClient::SetHidden hidden %u", hidden);

  for (auto it = browser_gst_map.begin(); it != browser_gst_map.end(); ++it)
  {
    if (!it->second)
    {
      GST_ERROR("BrowserClient::SetHidden, browser id: %d. UNABLE TO FIND it->second", it->first);
      continue;
    }

    if (it->second->gst_cef == gst_cef)
    {
      GstCefInfo_T *info = (GstCefInfo_T *)it->second;
      CefBrowser *browser = info->browser.get();
      CefRefPtr<CefBrowserHost> host = browser->GetHost();
      host->WasHidden(hidden);
      return;
    }
  }

  GST_INFO("Didn't get gst_cef");
}

void BrowserClient::ExecuteJS(void *gst_cef, char* js)
{
	// We need the UI thread because we are accessing the browser map.
	GST_INFO("Execute JS any thread");

	if (!CefCurrentlyOn(TID_UI))
	{
		// Execute on the UI thread.
		CefPostTask(TID_UI, base::Bind(&BrowserClient::ExecuteJS, this,
			gst_cef, js));
		return;
	}
	CEF_REQUIRE_UI_THREAD();

	GST_DEBUG("BrowserClient::ExecuteJS %s", js);

	for (auto it = browser_gst_map.begin(); it != browser_gst_map.end(); ++it)
	{
		if (!it->second)
		{
			GST_ERROR("BrowserClient::ExecuteJS, browser id: %d. UNABLE TO FIND it->second", it->first);
			continue;
		}

		if (it->second->gst_cef == gst_cef)
		{
			GstCefInfo_T *info = (GstCefInfo_T *)it->second;
			CefRefPtr<CefBrowser> browser = info->browser;
			CefRefPtr<CefFrame> frame = browser->GetMainFrame();
			frame->ExecuteJavaScript(CefString(js), frame->GetURL(), 0);
			g_free(js);
			return;
		}
	}

	g_free(js);
	GST_WARNING("Failed to execute javascript because we couldn't find the browser: %s", js);
}

void BrowserClient::SetSize(void *gst_cef, int width, int height)
{

  GST_INFO("Set Size Any Thread");

  if (!CefCurrentlyOn(TID_UI))
  {
    // Execute on the UI thread.
    CefPostTask(TID_UI, base::Bind(&BrowserClient::SetSize, this,
                                   gst_cef, width, height));
    return;
  }
  CEF_REQUIRE_UI_THREAD();

  GST_INFO("BrowserClient::Setting size of browser %ux%u", width, height);

  for (auto it = browser_gst_map.begin(); it != browser_gst_map.end(); ++it)
  {
    if (!it->second)
    {
      GST_ERROR("BrowserClient::SetSize, browser id: %d. UNABLE TO FIND it->second", it->first);
      continue;
    }

    if (it->second->gst_cef == gst_cef)
    {
      GstCefInfo_T *info = (GstCefInfo_T *)it->second;
      CefBrowser *browser = info->browser.get();
      GST_INFO("got cef. BrowserClient::SetSize id: %d", browser->GetIdentifier());
      info->width = width;
      info->height = height;
      CefBrowserHost *host = browser->GetHost().get();
      host->WasResized();
      return;
    }
  }

  GST_INFO("Didn't get gst_cef");
}
