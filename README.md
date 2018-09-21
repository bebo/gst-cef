

# CEF Plugin
### Requirements
* MSVC command line tools
* GStreamer 1.0
* GStreamer 1.0 development tools
* CMake
### Building

1. Point the CMake GUI at the project source code.
2. Uncheck the USE_SANDBOX variable, and change the GST_INSTALL_BASE to point at your gstreamer installation.
3. Press the "Configure", "Generate", and "Open Project" buttons.
4. In Visual Studio, change the build type to Release because Debug builds cause errors in CEF.
5. Build the solution.

### Testing

The .exe file that launches the gstreamer pipeline must be in the same directory as the cef resources.  Therefore, copy gst-launch-1.0 into the build/dist directory.

```
gst-launch-1.0 cef url="https://google.com" width=1280 height=720 ! autovideosink
```


```

    +-----------------------------------------------------------------------------------------------------+
    |                                                                                                     |
    |  GStreamer  (C)                                                                                     |
    |                                                                                                     |
    |                                                                                                     |
    |        +----------------+                                       +--------------+                    |
    |        |                |  1                                 n  |              |                    |
    |        |  GstCefClass   |  <---------------------------------+  |   GstCef     |                    |
    |        |                |                                       |              |                    |
    |        +----------------+                                       +--------------+                    |
    |                                                                                                     |
    |                                                                        +   + 1                      |
    |                                                                        |   |                        |
    |                        +-----------------------------------------------+                            |
    |                        |                                                   |                        |
    +-----------------------------------------------------------------------------------------------------+
                             |                                                   |
                +--------------------------+
                |   gst cef interface      |                                     |
                +--------------------------+
                             |                                                   |
    +--------------------------------------------------------------------------------------------------------------------------------------+
    |                        |                                                   |                                                         |
    |  Cef (C++)             |                                                                                                             |
    |                        |                                                   |                                                         |
    |                        v                                                   v 1                                                       |
    |                                                                                                                                      |
    |        +-----------------------------------+                    +---------------------------+            +---------------+           |
    |        |                                   |                    |                           | 1        1 |               |           |
    |        |  Browser -> App? BrowserProcess   |                    |  BrowserWindow            | +--------> |  CefBrowser   |           |
    |        |                                   |                    |                           |            |               |           |
    |        |                                   | 1                n |                           |            +---------------+           |
    |        |  * CefApp                         |                    |  * CefClient              |                                        |
    |        |  * CefBrowserProcessHandler       | +----------------> |  * CefDisplayHandler      |                                        |
    |        |  * CefRenderProcessHandler        |                    |  * CefLifeSpanHandler     |                                        |
    |        |                                   |                    |  * CefLoadHandler         |                                        |
    |        |                                   |                    |  * CefRenderHandler       |                                        |
    |        |  CloseBrowser()                   |                    |                           |                                        |
    |        |  CreateCefWindow()                |                    |  OnPaint()                |                                        |
    |        |  Refresh()                        |                    |  Refresh()                |                                        |
    |        |  Open()                           |                    |                           |                                        |
    |        |  OnBeforeCommandLineProcessing()  |                    |                           |                                        |
    |        |                                   |                    |                           |                                        |
    |        |                                   |                    |                           |                                        |
    |        |                                   |                    |                           |                                        |
    |        +-----------------------------------+                    +---------------------------+                                        |
    |                                                                                                                                      |
    |                                                                                                                                      |
    |                                                                                                                                      |
    +--------------------------------------------------------------------------------------------------------------------------------------+


```