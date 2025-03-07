// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_SSL_CLIENT_SOCKET_OPENSSL_H_
#define NET_SOCKET_SSL_CLIENT_SOCKET_OPENSSL_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "net/base/cert_verify_result.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/ssl_config_service.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/client_socket_handle.h"

typedef struct bio_st BIO;
typedef struct evp_pkey_st EVP_PKEY;
typedef struct ssl_st SSL;
typedef struct x509_st X509;

namespace net {

class CertVerifier;
class SingleRequestCertVerifier;
class SSLCertRequestInfo;
class SSLInfo;

// An SSL client socket implemented with OpenSSL.
class SSLClientSocketOpenSSL : public SSLClientSocket {
 public:
  // Takes ownership of the transport_socket, which may already be connected.
  // The given hostname will be compared with the name(s) in the server's
  // certificate during the SSL handshake.  ssl_config specifies the SSL
  // settings.
  SSLClientSocketOpenSSL(ClientSocketHandle* transport_socket,
                         const HostPortPair& host_and_port,
                         const SSLConfig& ssl_config,
                         const SSLClientSocketContext& context);
  virtual ~SSLClientSocketOpenSSL();

  const HostPortPair& host_and_port() const { return host_and_port_; }
  const std::string& ssl_session_cache_shard() const {
    return ssl_session_cache_shard_;
  }

  // Callback from the SSL layer that indicates the remote server is requesting
  // a certificate for this client.
  int ClientCertRequestCallback(SSL* ssl, X509** x509, EVP_PKEY** pkey);

  // Callback from the SSL layer to check which NPN protocol we are supporting
  int SelectNextProtoCallback(unsigned char** out, unsigned char* outlen,
                              const unsigned char* in, unsigned int inlen);

  // SSLClientSocket implementation.
  virtual void GetSSLCertRequestInfo(
      SSLCertRequestInfo* cert_request_info) override;
  virtual NextProtoStatus GetNextProto(std::string* proto,
                                       std::string* server_protos) override;
  virtual ServerBoundCertService* GetServerBoundCertService() const override;

  // SSLSocket implementation.
  virtual int ExportKeyingMaterial(const base::StringPiece& label,
                                   bool has_context,
                                   const base::StringPiece& context,
                                   unsigned char* out,
                                   unsigned int outlen) override;
  virtual int GetTLSUniqueChannelBinding(std::string* out) override;

  // StreamSocket implementation.
  virtual int Connect(const CompletionCallback& callback) override;
  virtual void Disconnect() override;
  virtual bool IsConnected() const override;
  virtual bool IsConnectedAndIdle() const override;
  virtual int GetPeerAddress(IPEndPoint* address) const override;
  virtual int GetLocalAddress(IPEndPoint* address) const override;
  virtual const BoundNetLog& NetLog() const override;
  virtual void SetSubresourceSpeculation() override;
  virtual void SetOmniboxSpeculation() override;
  virtual bool WasEverUsed() const override;
  virtual bool UsingTCPFastOpen() const override;
  virtual int64 NumBytesRead() const override;
  virtual base::TimeDelta GetConnectTimeMicros() const override;
  virtual bool GetSSLInfo(SSLInfo* ssl_info) override;

  // Socket implementation.
  virtual int Read(IOBuffer* buf, int buf_len,
                   const CompletionCallback& callback) override;
  virtual int Write(IOBuffer* buf, int buf_len,
                    const CompletionCallback& callback) override;
  virtual bool SetReceiveBufferSize(int32 size) override;
  virtual bool SetSendBufferSize(int32 size) override;

 private:
  bool Init();
  void DoReadCallback(int result);
  void DoWriteCallback(int result);

  bool DoTransportIO();
  int DoHandshake();
  int DoVerifyCert(int result);
  int DoVerifyCertComplete(int result);
  void DoConnectCallback(int result);
  X509Certificate* UpdateServerCert();

  void OnHandshakeIOComplete(int result);
  void OnSendComplete(int result);
  void OnRecvComplete(int result);

  int DoHandshakeLoop(int last_io_result);
  int DoReadLoop(int result);
  int DoWriteLoop(int result);
  int DoPayloadRead();
  int DoPayloadWrite();

  int BufferSend();
  int BufferRecv();
  void BufferSendComplete(int result);
  void BufferRecvComplete(int result);
  void TransportWriteComplete(int result);
  void TransportReadComplete(int result);

  bool transport_send_busy_;
  scoped_refptr<DrainableIOBuffer> send_buffer_;
  bool transport_recv_busy_;
  bool transport_recv_eof_;
  scoped_refptr<IOBuffer> recv_buffer_;

  CompletionCallback user_connect_callback_;
  CompletionCallback user_read_callback_;
  CompletionCallback user_write_callback_;

  base::WeakPtrFactory<SSLClientSocketOpenSSL> weak_factory_;

  // Used by Read function.
  scoped_refptr<IOBuffer> user_read_buf_;
  int user_read_buf_len_;

  // Used by Write function.
  scoped_refptr<IOBuffer> user_write_buf_;
  int user_write_buf_len_;

  // Set when handshake finishes.
  scoped_refptr<X509Certificate> server_cert_;
  CertVerifyResult server_cert_verify_result_;
  bool completed_handshake_;

  // Stores client authentication information between ClientAuthHandler and
  // GetSSLCertRequestInfo calls.
  std::vector<scoped_refptr<X509Certificate> > client_certs_;
  bool client_auth_cert_needed_;

  CertVerifier* const cert_verifier_;
  scoped_ptr<SingleRequestCertVerifier> verifier_;

  // OpenSSL stuff
  SSL* ssl_;
  BIO* transport_bio_;

  scoped_ptr<ClientSocketHandle> transport_;
  const HostPortPair host_and_port_;
  SSLConfig ssl_config_;
  // ssl_session_cache_shard_ is an opaque string that partitions the SSL
  // session cache. i.e. sessions created with one value will not attempt to
  // resume on the socket with a different value.
  const std::string ssl_session_cache_shard_;

  // Used for session cache diagnostics.
  bool trying_cached_session_;

  enum State {
    STATE_NONE,
    STATE_HANDSHAKE,
    STATE_VERIFY_CERT,
    STATE_VERIFY_CERT_COMPLETE,
  };
  State next_handshake_state_;
  NextProtoStatus npn_status_;
  std::string npn_proto_;
  std::string server_protos_;
  BoundNetLog net_log_;
};

}  // namespace net

#endif  // NET_SOCKET_SSL_CLIENT_SOCKET_OPENSSL_H_
