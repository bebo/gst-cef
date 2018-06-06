#include "file_scheme_handler.h"

#include <algorithm>
#include <iostream>

#include <include/cef_scheme.h>
#include <include/cef_parser.h>

FileSchemeHandler::FileSchemeHandler(): offset_(0), length_(0)
{
}

bool FileSchemeHandler::ProcessRequest(CefRefPtr<CefRequest> request,
    CefRefPtr<CefCallback> callback)
{
  CEF_REQUIRE_IO_THREAD();

  CefURLParts url_parts;
  CefParseURL(request->GetURL(), url_parts);

  std::string url_path = CefString(&url_parts.path).ToString();
  file_stream_.open(url_path, std::ifstream::in | std::ifstream::binary);

  // std::cout << "bebofile://url_path: " << url_path << "\n";

  if (!file_stream_.is_open()) {
    callback->Cancel();
    return false;
  }

  file_stream_.seekg(0, std::ios::end);
  length_ = file_stream_.tellg();
  file_stream_.seekg(0, std::ios::beg);

  // get mime_type by file extension
  size_t dot_index = url_path.find_last_of(".");
  if (dot_index != std::string::npos) {
    std::string extension = url_path.substr(url_path.find_last_of(".") + 1);
    mime_type_ = CefGetMimeType(extension);
  }

  callback->Continue();
  return true;
}

void FileSchemeHandler::GetResponseHeaders(CefRefPtr<CefResponse> response,
    int64& response_length,
    CefString& redirectUrl)
{
  CEF_REQUIRE_IO_THREAD();

  DCHECK(!data_.empty());

  response->SetMimeType(mime_type_);
  response->SetStatus(200);

  // Set the resulting response length.
  response_length = length_;
}

bool FileSchemeHandler::ReadResponse(void* data_out,
    int bytes_to_read,
    int& bytes_read,
    CefRefPtr<CefCallback> callback)
{
  CEF_REQUIRE_IO_THREAD();

  bytes_read = 0;

  if (!file_stream_.is_open()) {
    return false;
  }

  if (offset_ == length_) {
    file_stream_.close();
    return false;
  }

  file_stream_.read(reinterpret_cast<char*>(data_out), bytes_to_read);
  bytes_read = (int) file_stream_.gcount();
  offset_ += bytes_read;

  return true;
}

CefRefPtr<CefResourceHandler> FileSchemeHandlerFactory::Create(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    const CefString& scheme_name,
    CefRefPtr<CefRequest> request)
{
  CEF_REQUIRE_IO_THREAD();
  return new FileSchemeHandler();
}
