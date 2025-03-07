// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/dns_client.h"

#include "base/bind.h"
#include "base/rand_util.h"
#include "net/base/net_log.h"
#include "net/dns/address_sorter.h"
#include "net/dns/dns_config_service.h"
#include "net/dns/dns_session.h"
#include "net/dns/dns_socket_pool.h"
#include "net/dns/dns_transaction.h"
#include "net/socket/client_socket_factory.h"

namespace net {

namespace {

class DnsClientImpl : public DnsClient {
 public:
  explicit DnsClientImpl(NetLog* net_log)
      : address_sorter_(AddressSorter::CreateAddressSorter()),
        net_log_(net_log) {}

  virtual void SetConfig(const DnsConfig& config) override {
    factory_.reset();
    session_ = NULL;
    if (config.IsValid()) {
      ClientSocketFactory* factory = ClientSocketFactory::GetDefaultFactory();
      session_ = new DnsSession(config,
                                DnsSocketPool::CreateDefault(factory),
                                base::Bind(&base::RandInt),
                                net_log_);
      factory_ = DnsTransactionFactory::CreateFactory(session_);
    }
  }

  virtual const DnsConfig* GetConfig() const override {
    return session_.get() ? &session_->config() : NULL;
  }

  virtual DnsTransactionFactory* GetTransactionFactory() override {
    return session_.get() ? factory_.get() : NULL;
  }

  virtual AddressSorter* GetAddressSorter() override {
    return address_sorter_.get();
  }

 private:
  scoped_refptr<DnsSession> session_;
  scoped_ptr<DnsTransactionFactory> factory_;
  scoped_ptr<AddressSorter> address_sorter_;

  NetLog* net_log_;
};

}  // namespace

// static
scoped_ptr<DnsClient> DnsClient::CreateClient(NetLog* net_log) {
  return scoped_ptr<DnsClient>(new DnsClientImpl(net_log));
}

}  // namespace net

