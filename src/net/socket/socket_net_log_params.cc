// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/socket_net_log_params.h"

#include "base/bind.h"
#include "base/values.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_util.h"

namespace net {

namespace {

Value* NetLogSocketErrorCallback(int net_error,
                                 int os_error,
                                 NetLog::LogLevel /* log_level */) {
  DictionaryValue* dict = new DictionaryValue();
  dict->SetInteger("net_error", net_error);
  dict->SetInteger("os_error", os_error);
  return dict;
}

Value* NetLogHostPortPairCallback(const HostPortPair* host_and_port,
                                  NetLog::LogLevel /* log_level */) {
  DictionaryValue* dict = new DictionaryValue();
  dict->SetString("host_and_port", host_and_port->ToString());
  return dict;
}

Value* NetLogIPEndPointCallback(const IPEndPoint* address,
                                NetLog::LogLevel /* log_level */) {
  DictionaryValue* dict = new DictionaryValue();
  dict->SetString("address", address->ToString());
  return dict;
}

#if !defined(OS_STARBOARD)
Value* NetLogSourceAddressCallback(const struct sockaddr* net_address,
                                   socklen_t address_len,
                                   NetLog::LogLevel /* log_level */) {
  DictionaryValue* dict = new DictionaryValue();
  dict->SetString("source_address",
                  NetAddressToStringWithPort(net_address, address_len));
  return dict;
}
#endif

}  // namespace

NetLog::ParametersCallback CreateNetLogSocketErrorCallback(int net_error,
                                                           int os_error) {
  return base::Bind(&NetLogSocketErrorCallback, net_error, os_error);
}

NetLog::ParametersCallback CreateNetLogHostPortPairCallback(
    const HostPortPair* host_and_port) {
  return base::Bind(&NetLogHostPortPairCallback, host_and_port);
}

NetLog::ParametersCallback CreateNetLogIPEndPointCallback(
    const IPEndPoint* address) {
  return base::Bind(&NetLogIPEndPointCallback, address);
}

#if !defined(OS_STARBOARD)
NetLog::ParametersCallback CreateNetLogSourceAddressCallback(
    const struct sockaddr* net_address,
    socklen_t address_len) {
  return base::Bind(&NetLogSourceAddressCallback, net_address, address_len);
}
#endif

}  // namespace net
