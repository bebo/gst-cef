

# CEF Plugin

### Requirements Windows
* MSVC command line tools
* GStreamer 1.0
* GStreamer 1.0 development tools
* CMake

### Requirements Linux
* GStreamer 1.0
* GStreamer 1.0 development tools
* CMake >= 3.8
* build-essential
* if this is a headless machine - just install chromium to pull in all the needed runtime dependencies

cmake -G "Unix Makefiles" .. -DUSE_SANDBOX=0

# running it on linux
```
export LD_LIBRARY_PATH=/usr/local/cef/lib
export DISPLAY=:0
Xvfb :0 -screen 0 1280x720x16 &
```
The .exe file that launches the gstreamer pipeline must be in the same directory as the cef resources.  Therefore, copy gst-launch-1.0 into the build/dist directory.


### Testing


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
