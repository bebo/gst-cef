#ifndef _CEF_H_
#define _CEF_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct gstCb {
    void * gstCef;
    void * push_frame;
};

void browser_loop(void * args);

#ifdef __cplusplus
}
#endif


#if 0
void new_browser(void ** browser, const char * url, uint32_t width, uint32_t height, uint32_t fps, void * new_frame_callback);
void quit_browser();
#ifdef __cplusplus
class RenderHandler : public CefRenderHandler
{
private:
  int width;
  int height;
  uint8_t *frame_buffer;

public:

  RenderHandler(int width, int height);
  bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect);
  void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType paintType, const RectList &rects,
               const void *buffer, int width, int height) OVERRIDE;
  IMPLEMENT_REFCOUNTING(RenderHandler);
};

class BrowserClient : public CefClient {

public:
  BrowserClient(RenderHandler* renderHandler);
  ~BrowserClient();
  CefRefPtr<CefRenderHandler> GetRenderHandler() OVERRIDE;
private:
  CefRefPtr<CefRenderHandler> renderHandler;
  IMPLEMENT_REFCOUNTING(BrowserClient);
};


class Browser : public CefApp {
    public:
    bool init(const char * url, uint32_t width, uint32_t height, uint32_t fps);
    /* void update(); */
    void shutdown();
    ~Browser();
      void OnBeforeCommandLineProcessing(const CefString& process_type,
        CefRefPtr<CefCommandLine> command_line);
    IMPLEMENT_REFCOUNTING(Browser);
    private:
        CefRefPtr<RenderHandler> render_handler_;
        CefRefPtr<BrowserClient> browser_client_;
        CefRefPtr<CefBrowser> browser_;
};
#endif
#endif
#endif

