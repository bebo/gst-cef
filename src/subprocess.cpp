#include <iostream>
#include <Windows.h>

#include <include/cef_app.h>
#include "cef_app.h"

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PWSTR pCmdLine, int nCmdShow)
{
	CefMainArgs mainArgs(hInstance);
	// CefRefPtr<Browser> app(new Browser());
	// return CefExecuteProcess(mainArgs, app.get(), NULL);
	// TODO: Disable GPU
	return CefExecuteProcess(mainArgs, NULL, NULL);
}
