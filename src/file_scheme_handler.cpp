#include "file_scheme_handler.h"

#include <algorithm>
#include <iostream>

#include <include/cef_scheme.h>
#include <include/cef_parser.h>

void RegisterFileSchemeHandlerFactory(CefRawPtr<CefSchemeRegistrar> registrar)
{
  registrar->AddCustomScheme(kFileSchemeProtocol, true, false, false, true, true, false);
}

FileSchemeHandler::FileSchemeHandler(CefString bebofile_path)
  : offset_(0), length_(0), bebofile_path_(bebofile_path)
{
}

bool FileSchemeHandler::ProcessRequest(CefRefPtr<CefRequest> request,
    CefRefPtr<CefCallback> callback)
{
  CEF_REQUIRE_IO_THREAD();

  CefURLParts url_parts;
  CefParseURL(request->GetURL(), url_parts);

  cef_uri_unescape_rule_t unescape_rule = static_cast<cef_uri_unescape_rule_t>(
        UU_SPACES | UU_URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS);

  std::wstring decoded_host = CefURIDecode(CefString(&url_parts.host), false,
      unescape_rule);
  std::wstring decoded_path = CefURIDecode(CefString(&url_parts.path), false,
      unescape_rule);

  std::wstring url_path =  decoded_host + decoded_path;
  if (url_path.back() == '/') { // remove trailing slash
    url_path.pop_back();
  }
  std::wstring bf_path = bebofile_path_.ToWString();
  std::wstring path = bf_path + url_path;
  file_stream_.open(path, std::ifstream::in | std::ifstream::binary);

  // std::wcout << "resolved bebofs: " << path << "\n";

  if (!file_stream_.is_open()) {
    callback->Cancel();
    return false;
  }

  file_stream_.seekg(0, std::ios::end);
  length_ = file_stream_.tellg();
  file_stream_.seekg(0, std::ios::beg);

  // get mime_type by file extension
  size_t dot_index = path.find_last_of(L".");
  if (dot_index != std::string::npos) {
    std::wstring extension = path.substr(dot_index + 1);
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
  return new FileSchemeHandler(bebofile_path_);
}

