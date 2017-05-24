#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/cef_render_handler.h>
#include <include/wrapper/cef_helpers.h>
/* #include "cef.h" */
// Program entry-point function.
int main(int argc, char* argv[]) {
  // Structure for passing command-line arguments.
  // The definition of this structure is platform-specific.
  CefMainArgs main_args(argc, argv);

  // Optional implementation of the CefApp interface.
  /* CefRefPtr<Browser> app(new Browser); */

  // Execute the sub-process logic. This will block until the sub-process should exit.
  return CefExecuteProcess(main_args, nullptr, nullptr);
}
