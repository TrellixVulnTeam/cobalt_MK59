// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_response_body_drainer.h"

#include <cstring>

#include "base/bind.h"
#include "base/compiler_specific.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/ssl_config_service_defaults.h"
#include "net/base/test_completion_callback.h"
#include "net/http/http_network_session.h"
#include "net/http/http_server_properties_impl.h"
#include "net/http/http_stream.h"
#include "net/proxy/proxy_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

const int kMagicChunkSize = 1024;
COMPILE_ASSERT(
    (HttpResponseBodyDrainer::kDrainBodyBufferSize % kMagicChunkSize) == 0,
    chunk_size_needs_to_divide_evenly_into_buffer_size);

class CloseResultWaiter {
 public:
  CloseResultWaiter()
      : result_(false),
        have_result_(false),
        waiting_for_result_(false) {}

  int WaitForResult() {
    CHECK(!waiting_for_result_);
    while (!have_result_) {
      waiting_for_result_ = true;
      MessageLoop::current()->Run();
      waiting_for_result_ = false;
    }
    return result_;
  }

  void set_result(bool result) {
    result_ = result;
    have_result_ = true;
    if (waiting_for_result_)
      MessageLoop::current()->Quit();
  }

 private:
  int result_;
  bool have_result_;
  bool waiting_for_result_;

  DISALLOW_COPY_AND_ASSIGN(CloseResultWaiter);
};

class MockHttpStream : public HttpStream {
 public:
  MockHttpStream(CloseResultWaiter* result_waiter)
      : result_waiter_(result_waiter),
        buf_len_(0),
        closed_(false),
        stall_reads_forever_(false),
        num_chunks_(0),
        is_sync_(false),
        is_last_chunk_zero_size_(false),
        is_complete_(false),
        ALLOW_THIS_IN_INITIALIZER_LIST(weak_factory_(this)) {}
  virtual ~MockHttpStream() {}

  // HttpStream implementation.
  virtual int InitializeStream(const HttpRequestInfo* request_info,
                               const BoundNetLog& net_log,
                               const CompletionCallback& callback) override {
    return ERR_UNEXPECTED;
  }
  virtual int SendRequest(const HttpRequestHeaders& request_headers,
                          HttpResponseInfo* response,
                          const CompletionCallback& callback) override {
    return ERR_UNEXPECTED;
  }
  virtual UploadProgress GetUploadProgress() const override {
    return UploadProgress();
  }
  virtual int ReadResponseHeaders(const CompletionCallback& callback) override {
    return ERR_UNEXPECTED;
  }
  virtual const HttpResponseInfo* GetResponseInfo() const override {
    return NULL;
  }

  virtual bool CanFindEndOfResponse() const override { return true; }
  virtual bool IsMoreDataBuffered() const override { return false; }
  virtual bool IsConnectionReused() const override { return false; }
  virtual void SetConnectionReused() override {}
  virtual bool IsConnectionReusable() const override { return false; }
  virtual void GetSSLInfo(SSLInfo* ssl_info) override {}
  virtual void GetSSLCertRequestInfo(
      SSLCertRequestInfo* cert_request_info) override {}

  // Mocked API
  virtual int ReadResponseBody(IOBuffer* buf, int buf_len,
                               const CompletionCallback& callback) override;
  virtual void Close(bool not_reusable) override {
    CHECK(!closed_);
    closed_ = true;
    result_waiter_->set_result(not_reusable);
  }

  virtual HttpStream* RenewStreamForAuth() override {
    return NULL;
  }

  virtual bool IsResponseBodyComplete() const override { return is_complete_; }

  virtual bool IsSpdyHttpStream() const override { return false; }

  virtual void LogNumRttVsBytesMetrics() const override {}

  virtual void Drain(HttpNetworkSession*) override {}

  // Methods to tweak/observer mock behavior:
  void set_stall_reads_forever() { stall_reads_forever_ = true; }

  void set_num_chunks(int num_chunks) { num_chunks_ = num_chunks; }

  void set_sync() { is_sync_ = true; }

  void set_is_last_chunk_zero_size() { is_last_chunk_zero_size_ = true; }

 private:
  int ReadResponseBodyImpl(IOBuffer* buf, int buf_len);
  void CompleteRead();

  bool closed() const { return closed_; }

  CloseResultWaiter* const result_waiter_;
  scoped_refptr<IOBuffer> user_buf_;
  CompletionCallback callback_;
  int buf_len_;
  bool closed_;
  bool stall_reads_forever_;
  int num_chunks_;
  bool is_sync_;
  bool is_last_chunk_zero_size_;
  bool is_complete_;
  base::WeakPtrFactory<MockHttpStream> weak_factory_;
};

int MockHttpStream::ReadResponseBody(IOBuffer* buf,
                                     int buf_len,
                                     const CompletionCallback& callback) {
  CHECK(!callback.is_null());
  CHECK(callback_.is_null());
  CHECK(buf);

  if (stall_reads_forever_)
    return ERR_IO_PENDING;

  if (is_complete_)
    return ERR_UNEXPECTED;

  if (!is_sync_) {
    user_buf_ = buf;
    buf_len_ = buf_len;
    callback_ = callback;
    MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&MockHttpStream::CompleteRead, weak_factory_.GetWeakPtr()));
    return ERR_IO_PENDING;
  } else {
    return ReadResponseBodyImpl(buf, buf_len);
  }
}

int MockHttpStream::ReadResponseBodyImpl(IOBuffer* buf, int buf_len) {
  if (is_last_chunk_zero_size_ && num_chunks_ == 1) {
    buf_len = 0;
  } else {
    if (buf_len > kMagicChunkSize)
      buf_len = kMagicChunkSize;
    std::memset(buf->data(), 1, buf_len);
  }
  num_chunks_--;
  if (!num_chunks_)
    is_complete_ = true;

  return buf_len;
}

void MockHttpStream::CompleteRead() {
  int result = ReadResponseBodyImpl(user_buf_, buf_len_);
  user_buf_ = NULL;
  CompletionCallback callback = callback_;
  callback_.Reset();
  callback.Run(result);
}

class HttpResponseBodyDrainerTest : public testing::Test {
 protected:
  HttpResponseBodyDrainerTest()
      : proxy_service_(ProxyService::CreateDirect()),
        ssl_config_service_(new SSLConfigServiceDefaults),
        http_server_properties_(new HttpServerPropertiesImpl),
        session_(CreateNetworkSession()),
        mock_stream_(new MockHttpStream(&result_waiter_)),
        drainer_(new HttpResponseBodyDrainer(mock_stream_)) {}

  ~HttpResponseBodyDrainerTest() {}

  HttpNetworkSession* CreateNetworkSession() const {
    HttpNetworkSession::Params params;
    params.proxy_service = proxy_service_.get();
    params.ssl_config_service = ssl_config_service_;
    params.http_server_properties = http_server_properties_.get();
    return new HttpNetworkSession(params);
  }

  scoped_ptr<ProxyService> proxy_service_;
  scoped_refptr<SSLConfigService> ssl_config_service_;
  scoped_ptr<HttpServerPropertiesImpl> http_server_properties_;
  const scoped_refptr<HttpNetworkSession> session_;
  CloseResultWaiter result_waiter_;
  MockHttpStream* const mock_stream_;  // Owned by |drainer_|.
  HttpResponseBodyDrainer* const drainer_;  // Deletes itself.
};

TEST_F(HttpResponseBodyDrainerTest, DrainBodySyncSingleOK) {
  mock_stream_->set_num_chunks(1);
  mock_stream_->set_sync();
  drainer_->Start(session_);
  EXPECT_FALSE(result_waiter_.WaitForResult());
}

TEST_F(HttpResponseBodyDrainerTest, DrainBodySyncOK) {
  mock_stream_->set_num_chunks(3);
  mock_stream_->set_sync();
  drainer_->Start(session_);
  EXPECT_FALSE(result_waiter_.WaitForResult());
}

TEST_F(HttpResponseBodyDrainerTest, DrainBodyAsyncOK) {
  mock_stream_->set_num_chunks(3);
  drainer_->Start(session_);
  EXPECT_FALSE(result_waiter_.WaitForResult());
}

// Test the case when the final chunk is 0 bytes. This can happen when
// the final 0-byte chunk of a chunk-encoded http response is read in a last
// call to ReadResponseBody, after all data were returned from HttpStream.
TEST_F(HttpResponseBodyDrainerTest, DrainBodyAsyncEmptyChunk) {
  mock_stream_->set_num_chunks(4);
  mock_stream_->set_is_last_chunk_zero_size();
  drainer_->Start(session_);
  EXPECT_FALSE(result_waiter_.WaitForResult());
}

TEST_F(HttpResponseBodyDrainerTest, DrainBodySyncEmptyChunk) {
  mock_stream_->set_num_chunks(4);
  mock_stream_->set_sync();
  mock_stream_->set_is_last_chunk_zero_size();
  drainer_->Start(session_);
  EXPECT_FALSE(result_waiter_.WaitForResult());
}

TEST_F(HttpResponseBodyDrainerTest, DrainBodySizeEqualsDrainBuffer) {
  mock_stream_->set_num_chunks(
      HttpResponseBodyDrainer::kDrainBodyBufferSize / kMagicChunkSize);
  drainer_->Start(session_);
  EXPECT_FALSE(result_waiter_.WaitForResult());
}

TEST_F(HttpResponseBodyDrainerTest, DrainBodyTimeOut) {
  mock_stream_->set_num_chunks(2);
  mock_stream_->set_stall_reads_forever();
  drainer_->Start(session_);
  EXPECT_TRUE(result_waiter_.WaitForResult());
}

TEST_F(HttpResponseBodyDrainerTest, CancelledBySession) {
  mock_stream_->set_num_chunks(2);
  mock_stream_->set_stall_reads_forever();
  drainer_->Start(session_);
  // HttpNetworkSession should delete |drainer_|.
}

TEST_F(HttpResponseBodyDrainerTest, DrainBodyTooLarge) {
  int too_many_chunks =
      HttpResponseBodyDrainer::kDrainBodyBufferSize / kMagicChunkSize;
  too_many_chunks += 1;  // Now it's too large.

  mock_stream_->set_num_chunks(too_many_chunks);
  drainer_->Start(session_);
  EXPECT_TRUE(result_waiter_.WaitForResult());
}

TEST_F(HttpResponseBodyDrainerTest, StartBodyTooLarge) {
  int too_many_chunks =
      HttpResponseBodyDrainer::kDrainBodyBufferSize / kMagicChunkSize;
  too_many_chunks += 1;  // Now it's too large.

  mock_stream_->set_num_chunks(0);
  drainer_->StartWithSize(session_, too_many_chunks * kMagicChunkSize);
  EXPECT_TRUE(result_waiter_.WaitForResult());
}

TEST_F(HttpResponseBodyDrainerTest, StartWithNothingToDo) {
  mock_stream_->set_num_chunks(0);
  drainer_->StartWithSize(session_, 0);
  EXPECT_FALSE(result_waiter_.WaitForResult());
}

}  // namespace

}  // namespace net
