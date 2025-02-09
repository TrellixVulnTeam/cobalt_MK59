// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_NETWORK_SESSION_H_
#define NET_HTTP_HTTP_NETWORK_SESSION_H_

#include <set>
#include "base/memory/ref_counted.h"
#include "base/threading/non_thread_safe.h"
#include "net/base/host_port_pair.h"
#include "net/base/host_resolver.h"
#include "net/base/net_export.h"
#include "net/base/ssl_client_auth_cache.h"
#include "net/http/http_auth_cache.h"
#include "net/http/http_stream_factory.h"
#include "net/quic/quic_stream_factory.h"
#include "net/spdy/spdy_session_pool.h"

namespace base {
class Value;
}

namespace net {

class CertVerifier;
class ClientSocketFactory;
class ClientSocketPoolManager;
class HostResolver;
class HttpAuthHandlerFactory;
class HttpNetworkSessionPeer;
class HttpProxyClientSocketPool;
class HttpResponseBodyDrainer;
class HttpServerProperties;
class NetLog;
class NetworkDelegate;
class ServerBoundCertService;
class ProxyService;
class SOCKSClientSocketPool;
class SSLClientSocketPool;
class SSLConfigService;
class TransportClientSocketPool;
class TransportSecurityState;

// This class holds session objects used by HttpNetworkTransaction objects.
class NET_EXPORT HttpNetworkSession
    : public base::RefCounted<HttpNetworkSession>,
      NON_EXPORTED_BASE(public base::NonThreadSafe) {
 public:
  struct NET_EXPORT Params {
    Params();

    ClientSocketFactory* client_socket_factory;
    HostResolver* host_resolver;
    CertVerifier* cert_verifier;
    ServerBoundCertService* server_bound_cert_service;
    TransportSecurityState* transport_security_state;
    ProxyService* proxy_service;
    std::string ssl_session_cache_shard;
    SSLConfigService* ssl_config_service;
    HttpAuthHandlerFactory* http_auth_handler_factory;
    NetworkDelegate* network_delegate;
    HttpServerProperties* http_server_properties;
    NetLog* net_log;
    HostMappingRules* host_mapping_rules;
    bool force_http_pipelining;
    bool ignore_certificate_errors;
    bool http_pipelining_enabled;
    uint16 testing_fixed_http_port;
    uint16 testing_fixed_https_port;
    size_t max_spdy_sessions_per_domain;
    bool force_spdy_single_domain;
    bool enable_spdy_ip_pooling;
    bool enable_spdy_credential_frames;
    bool enable_spdy_compression;
    bool enable_spdy_ping_based_connection_checking;
    NextProto spdy_default_protocol;
    size_t spdy_initial_recv_window_size;
    size_t spdy_initial_max_concurrent_streams;
    size_t spdy_max_concurrent_streams_limit;
    SpdySessionPool::TimeFunc time_func;
    std::string trusted_spdy_proxy;
    uint16 origin_port_to_force_quic_on;
  };

  enum SocketPoolType {
    NORMAL_SOCKET_POOL,
    WEBSOCKET_SOCKET_POOL,
    NUM_SOCKET_POOL_TYPES
  };

  explicit HttpNetworkSession(const Params& params);

  HttpAuthCache* http_auth_cache() { return &http_auth_cache_; }
  SSLClientAuthCache* ssl_client_auth_cache() {
    return &ssl_client_auth_cache_;
  }

  void AddResponseDrainer(HttpResponseBodyDrainer* drainer);

  void RemoveResponseDrainer(HttpResponseBodyDrainer* drainer);

  TransportClientSocketPool* GetTransportSocketPool(SocketPoolType pool_type);
  SSLClientSocketPool* GetSSLSocketPool(SocketPoolType pool_type);
  SOCKSClientSocketPool* GetSocketPoolForSOCKSProxy(
      SocketPoolType pool_type,
      const HostPortPair& socks_proxy);
  HttpProxyClientSocketPool* GetSocketPoolForHTTPProxy(
      SocketPoolType pool_type,
      const HostPortPair& http_proxy);
  SSLClientSocketPool* GetSocketPoolForSSLWithProxy(
      SocketPoolType pool_type,
      const HostPortPair& proxy_server);

  CertVerifier* cert_verifier() { return cert_verifier_; }
  ProxyService* proxy_service() { return proxy_service_; }
  SSLConfigService* ssl_config_service() { return ssl_config_service_; }

  SpdySessionPool* spdy_session_pool() {
#if defined(COBALT_DISABLE_SPDY)
    return NULL;
#else
    return &spdy_session_pool_;
#endif  // defined(COBALT_DISABLE_SPDY)
  }

  QuicStreamFactory* quic_stream_factory() { return &quic_stream_factory_; }
  HttpAuthHandlerFactory* http_auth_handler_factory() {
    return http_auth_handler_factory_;
  }
  NetworkDelegate* network_delegate() {
    return network_delegate_;
  }
  HttpServerProperties* http_server_properties() {
    return http_server_properties_;
  }
  HttpStreamFactory* http_stream_factory() {
    return http_stream_factory_.get();
  }
  NetLog* net_log() {
    return net_log_;
  }

  // Creates a Value summary of the state of the socket pools. The caller is
  // responsible for deleting the returned value.
  base::Value* SocketPoolInfoToValue() const;

  // Creates a Value summary of the state of the SPDY sessions. The caller is
  // responsible for deleting the returned value.
  base::Value* SpdySessionPoolInfoToValue() const;

  void CloseAllConnections();
  void CloseIdleConnections();

  bool force_http_pipelining() const { return force_http_pipelining_; }

  // Returns the original Params used to construct this session.
  const Params& params() const { return params_; }

  void set_http_pipelining_enabled(bool enable) {
    params_.http_pipelining_enabled = enable;
  }

 private:
  friend class base::RefCounted<HttpNetworkSession>;
  friend class HttpNetworkSessionPeer;

  ~HttpNetworkSession();

  ClientSocketPoolManager* GetSocketPoolManager(SocketPoolType pool_type);

  NetLog* const net_log_;
  NetworkDelegate* const network_delegate_;
  HttpServerProperties* const http_server_properties_;
  CertVerifier* const cert_verifier_;
  HttpAuthHandlerFactory* const http_auth_handler_factory_;
  bool force_http_pipelining_;

  // Not const since it's modified by HttpNetworkSessionPeer for testing.
  ProxyService* proxy_service_;
  const scoped_refptr<SSLConfigService> ssl_config_service_;

  HttpAuthCache http_auth_cache_;
  SSLClientAuthCache ssl_client_auth_cache_;
  scoped_ptr<ClientSocketPoolManager> normal_socket_pool_manager_;
  scoped_ptr<ClientSocketPoolManager> websocket_socket_pool_manager_;
  QuicStreamFactory quic_stream_factory_;
#if !defined(COBALT_DISABLE_SPDY)
  SpdySessionPool spdy_session_pool_;
#endif  // !defined(COBALT_DISABLE_SPDY)
  scoped_ptr<HttpStreamFactory> http_stream_factory_;
  std::set<HttpResponseBodyDrainer*> response_drainers_;

  Params params_;
};

}  // namespace net

#endif  // NET_HTTP_HTTP_NETWORK_SESSION_H_
