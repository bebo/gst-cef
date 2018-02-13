#include <Windows.h>

#include <include/cef_app.h>

// Program entry-point function.
int main(int argc, char *argv[]) {
  // Structure for passing command-line arguments.
  // The definition of this structure is platform-specific.
  CefMainArgs main_args(GetModuleHandle(NULL));

  // Optional implementation of the CefApp interface.
  /* CefRefPtr<Browser> app(new Browser); */

  // Execute the sub-process logic. This will block until the sub-process should exit.
  return CefExecuteProcess(main_args, nullptr, nullptr);
}
