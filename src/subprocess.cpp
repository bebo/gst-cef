#include <iostream>
#ifdef _WIN32
#include <Windows.h>
#endif

#include <include/cef_app.h>
#include "file_scheme_handler.h"

class BrowserApp : public CefApp
{
public:
  BrowserApp();

  virtual void OnBeforeCommandLineProcessing(
      const CefString &process_type,
      CefRefPtr<CefCommandLine> command_line) OVERRIDE;

  virtual void OnRegisterCustomSchemes(
    CefRawPtr<CefSchemeRegistrar> registrar) OVERRIDE;

  IMPLEMENT_REFCOUNTING(BrowserApp);
};

BrowserApp::BrowserApp() {}

void BrowserApp::OnBeforeCommandLineProcessing(
    const CefString &process_type,
    CefRefPtr<CefCommandLine> command_line)
{
  command_line->AppendSwitch("disable-gpu");
  command_line->AppendSwitch("disable-gpu-compositing");
  command_line->AppendSwitch("enable-begin-frame-scheduling");
  command_line->AppendSwitch("enable-system-flash");
  command_line->AppendSwitch("log-severity=disable");
}

void BrowserApp::OnRegisterCustomSchemes(
    CefRawPtr<CefSchemeRegistrar> registrar)
{
  registrar->AddCustomScheme(kFileSchemeProtocol, true, false, false, true, true, false);
}

#ifdef _WIN32
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      PWSTR pCmdLine, int nCmdShow)
{
  CefMainArgs mainArgs(hInstance);
  CefRefPtr<BrowserApp> app(new BrowserApp());
  return CefExecuteProcess(mainArgs, app.get(), NULL);
}

#else

int main(int argc, char* argv[]) {
  // Structure for passing command-line arguments.
  // The definition of this structure is platform-specific.
  CefMainArgs mainArgs(argc, argv);

  // Optional implementation of the CefApp interface.
  CefRefPtr<BrowserApp> app(new BrowserApp());

  // Execute the sub-process logic. This will block until the sub-process should exit.
  return CefExecuteProcess(mainArgs, app.get(), NULL);
}
#endif
