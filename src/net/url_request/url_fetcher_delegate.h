// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_URL_REQUEST_URL_FETCHER_DELEGATE_H_
#define NET_URL_REQUEST_URL_FETCHER_DELEGATE_H_

#include <string>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "net/base/net_export.h"

namespace net {

class URLFetcher;

// A delegate interface for users of URLFetcher.
class NET_EXPORT URLFetcherDelegate {
 public:
#if defined(COBALT)
  // This will be called when the response code and headers have been received.
  virtual void OnURLFetchResponseStarted(const URLFetcher* source);
#endif  // defined(COBALT)

  // This will be called when the URL has been fetched, successfully or not.
  // Use accessor methods on |source| to get the results.
  virtual void OnURLFetchComplete(const URLFetcher* source) = 0;

  // This will be called when some part of the response is read.
  // |download_data| contains the current bytes received since the last call.
  // This will be called after ShouldSendDownloadData() and only if the latter
  // returns true.
  virtual void OnURLFetchDownloadData(const URLFetcher* source,
                                      scoped_ptr<std::string> download_data);

  // This indicates if OnURLFetchDownloadData should be called.
  // This will be called before OnURLFetchDownloadData is called, and only if
  // this returns true.
  // Default implementation is false.
  virtual bool ShouldSendDownloadData();

  // This will be called when uploading of POST or PUT requests proceeded.
  // |current| denotes the number of bytes sent so far, and |total| is the
  // total size of uploading data (or -1 if chunked upload is enabled).
  virtual void OnURLFetchUploadProgress(const URLFetcher* source,
                                        int64 current, int64 total);

 protected:
  virtual ~URLFetcherDelegate();
};

}  // namespace net

#endif  // NET_URL_REQUEST_URL_FETCHER_DELEGATE_H_
