// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/url_request/file_protocol_handler.h"

#include "base/logging.h"
#include "net/base/net_errors.h"
#include "net/base/net_util.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_error_job.h"
#if !defined(COBALT)
#include "net/url_request/url_request_file_dir_job.h"
#endif
#include "net/url_request/url_request_file_job.h"

namespace net {

FileProtocolHandler::FileProtocolHandler() { }

URLRequestJob* FileProtocolHandler::MaybeCreateJob(
    URLRequest* request, NetworkDelegate* network_delegate) const {
  FilePath file_path;
  const bool is_file = FileURLToFilePath(request->url(), &file_path);

  // Check file access permissions.
  if (!network_delegate ||
      !network_delegate->CanAccessFile(*request, file_path)) {
    return new URLRequestErrorJob(request, network_delegate, ERR_ACCESS_DENIED);
  }
#if !defined (COBALT) // Cobalt doesn't support URLRequestFileDirJob
  // We need to decide whether to create URLRequestFileJob for file access or
  // URLRequestFileDirJob for directory access. To avoid accessing the
  // filesystem, we only look at the path string here.
  // The code in the URLRequestFileJob::Start() method discovers that a path,
  // which doesn't end with a slash, should really be treated as a directory,
  // and it then redirects to the URLRequestFileDirJob.
  if (is_file &&
      file_util::EndsWithSeparator(file_path) &&
      file_path.IsAbsolute()) {
    return new URLRequestFileDirJob(request, network_delegate, file_path);
  }
#endif
  // Use a regular file request job for all non-directories (including invalid
  // file names).
  return new URLRequestFileJob(request, network_delegate, file_path);
}

}  // namespace net
