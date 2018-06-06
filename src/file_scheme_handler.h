#ifndef FILE_SCHEME_HANDLER_FACTORY_H_
#define FILE_SCHEME_HANDLER_FACTORY_H_

#include <fstream>
#include "include/cef_app.h"
#include "include/wrapper/cef_helpers.h"

// Implementation of the scheme handler for file:// requests.
class FileSchemeHandler : public CefResourceHandler {
 public:
  FileSchemeHandler();

  void Cancel() OVERRIDE { CEF_REQUIRE_IO_THREAD(); }

  virtual bool ProcessRequest(CefRefPtr<CefRequest> request,
      CefRefPtr<CefCallback> callback) OVERRIDE;

  virtual void GetResponseHeaders(CefRefPtr<CefResponse> response,
      int64& response_length,
      CefString& redirectUrl) OVERRIDE;

  virtual bool ReadResponse(void* data_out,
      int bytes_to_read,
      int& bytes_read,
      CefRefPtr<CefCallback> callback) OVERRIDE;

 private:
  std::ifstream file_stream_;
  std::string data_;
  std::string mime_type_;
  size_t offset_;
  size_t length_;


  IMPLEMENT_REFCOUNTING(FileSchemeHandler);
  DISALLOW_COPY_AND_ASSIGN(FileSchemeHandler);
};

class FileSchemeHandlerFactory : public CefSchemeHandlerFactory
{
public:
  FileSchemeHandlerFactory() {}

  virtual CefRefPtr<CefResourceHandler> Create(CefRefPtr<CefBrowser> browser,
      CefRefPtr<CefFrame> frame,
      const CefString& scheme_name,
      CefRefPtr<CefRequest> request) OVERRIDE;

private:
  IMPLEMENT_REFCOUNTING(FileSchemeHandlerFactory);
};

#endif // FILE_SCHEME_HANDLER_FACTORY_H_
