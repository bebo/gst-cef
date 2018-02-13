#include <iostream>
#include <Windows.h>

#include <include/cef_app.h>
#include "cef_app.h"

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	CefMainArgs mainArgs(NULL);
	// CefRefPtr<Browser> app(new Browser);
	return CefExecuteProcess(mainArgs, NULL, NULL);
}