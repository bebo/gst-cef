#include "file_scheme_handler.h"

#include <algorithm>
#include <iostream>

#ifndef __WIN
#include <string>
#include <locale>
#include <codecvt>
#include <iomanip>
#endif

#include <include/cef_scheme.h>
#include <include/cef_parser.h>

void RegisterFileSchemeHandlerFactory(CefRawPtr<CefSchemeRegistrar> registrar)
{
  registrar->AddCustomScheme(kFileSchemeProtocol, true, false, false, true, true, false);
}

FileSchemeHandler::FileSchemeHandler(CefString local_filepath)
  : offset_(0), length_(0), local_filepath_(local_filepath)
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
  std::wstring bf_path = local_filepath_.ToWString();
  if (bf_path.back() != '/') {
    bf_path.push_back(L'/');
  }

#ifdef __WIN
  std::wstring path = bf_path + url_path;
#else
  std::wstring_convert<std::codecvt_utf8<wchar_t>> conv1;
  std::string path = conv1.to_bytes(bf_path + url_path);
#endif
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
  size_t dot_index = url_path.find_last_of(L".");
  if (dot_index != std::string::npos) {
    std::wstring extension = url_path.substr(dot_index + 1);
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

  response->SetMimeType(mime_type_);
  response->SetStatus(200);
  response->SetStatusText("OK");

  // Set the resulting response length.
  response_length = length_;
  redirectUrl = "";
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

  file_stream_.read(reinterpret_cast<char*>(data_out), bytes_to_read);
  bytes_read = (int) file_stream_.gcount();
  offset_ += bytes_read;

  if (offset_ == length_) {
    file_stream_.close();
  }

  return true;
}

void FileSchemeHandler::Cancel() { 
  CEF_REQUIRE_IO_THREAD();
  if (file_stream_.is_open()) {
    file_stream_.close();
  }
  
  // std::cout << "offset: " << offset_ << ", length: " << length_ << "\n";
}

CefRefPtr<CefResourceHandler> FileSchemeHandlerFactory::Create(
    CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame,
    const CefString& scheme_name,
    CefRefPtr<CefRequest> request)
{
  CEF_REQUIRE_IO_THREAD();
  return new FileSchemeHandler(local_filepath_);
}

