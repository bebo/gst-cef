#include <iostream>
#include <Windows.h>

#include <include/cef_app.h>

class BrowserApp : public CefApp {
public:
  BrowserApp();

  virtual void OnBeforeCommandLineProcessing(
    const CefString& process_type,
    CefRefPtr<CefCommandLine> command_line) OVERRIDE;

  IMPLEMENT_REFCOUNTING(BrowserApp);
};

BrowserApp::BrowserApp() {}

void BrowserApp::OnBeforeCommandLineProcessing(
  const CefString& process_type,
  CefRefPtr<CefCommandLine> command_line) {
  command_line->AppendSwitch("disable-gpu");
  command_line->AppendSwitch("disable-gpu-compositing");
  command_line->AppendSwitch("enable-begin-frame-scheduling");
  command_line->AppendSwitch("enable-system-flash");
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
  PWSTR pCmdLine, int nCmdShow)
{
  CefMainArgs mainArgs(hInstance);
  CefRefPtr<BrowserApp> app(new BrowserApp());
  return CefExecuteProcess(mainArgs, app.get(), NULL);
}
