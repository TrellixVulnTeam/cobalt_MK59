// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/address_list.h"

#include "base/string_util.h"
#include "base/sys_byteorder.h"
#include "build/build_config.h"
#include "net/base/net_util.h"
#include "net/base/sys_addrinfo.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_STARBOARD)
#include "starboard/memory.h"
#include "starboard/socket.h"
#endif

namespace net {
namespace {

static const char* kCanonicalHostname = "canonical.bar.com";

#if defined(OS_STARBOARD)
TEST(AddressListTest, CreateFromSbSocketResolution) {
  // Create an 4-element addrinfo.
  const unsigned kNumElements = 4;
  SbSocketResolution resolution = {0};
  resolution.address_count = kNumElements;
  struct SbSocketAddress addresses[kNumElements];
  resolution.addresses = addresses;
  for (int i = 0; i < kNumElements; ++i) {
    SbSocketAddress* address = &addresses[i];
    // Populating the address with { i, i, i, i }.
    SbMemorySet(address->address, i, kIPv4AddressSize);
    address->type = kSbSocketAddressTypeIpv4;
    // Set port to i << 2;
    address->port = i << 2;
  }

  AddressList list = AddressList::CreateFromSbSocketResolution(&resolution);

  ASSERT_EQ(kNumElements, list.size());
  for (size_t i = 0; i < list.size(); ++i) {
    EXPECT_EQ(ADDRESS_FAMILY_IPV4, list[i].GetFamily());
    // Only check the first byte of the address.
    EXPECT_EQ(i, list[i].address()[0]);
    EXPECT_EQ(static_cast<int>(i << 2), list[i].port());
  }

  // Check if operator= works.
  AddressList copy;
  copy = list;
  ASSERT_EQ(kNumElements, copy.size());

  // Check if copy is independent.
  copy[1] = IPEndPoint(copy[2].address(), 0xBEEF);
  // Original should be unchanged.
  EXPECT_EQ(1u, list[1].address()[0]);
  EXPECT_EQ(1 << 2, list[1].port());
}
#else   // defined(OS_STARBOARD)
TEST(AddressListTest, Canonical) {
  // Create an addrinfo with a canonical name.
  struct sockaddr_in address;
  // The contents of address do not matter for this test,
  // so just zero-ing them out for consistency.
  memset(&address, 0x0, sizeof(address));
  // But we need to set the family.
  address.sin_family = AF_INET;
  struct addrinfo ai;
  memset(&ai, 0x0, sizeof(ai));
  ai.ai_family = AF_INET;
  ai.ai_socktype = SOCK_STREAM;
  ai.ai_addrlen = sizeof(address);
  ai.ai_addr = reinterpret_cast<sockaddr*>(&address);
  ai.ai_canonname = const_cast<char *>(kCanonicalHostname);

  // Copy the addrinfo struct into an AddressList object and
  // make sure it seems correct.
  AddressList addrlist1 = AddressList::CreateFromAddrinfo(&ai);
  EXPECT_EQ("canonical.bar.com", addrlist1.canonical_name());

  // Copy the AddressList to another one.
  AddressList addrlist2 = addrlist1;
  EXPECT_EQ("canonical.bar.com", addrlist2.canonical_name());
}

TEST(AddressListTest, CreateFromAddrinfo) {
  // Create an 4-element addrinfo.
  const unsigned kNumElements = 4;
  SockaddrStorage storage[kNumElements];
  struct addrinfo ai[kNumElements];
  for (unsigned i = 0; i < kNumElements; ++i) {
    struct sockaddr_in* addr =
        reinterpret_cast<struct sockaddr_in*>(storage[i].addr);
    storage[i].addr_len = sizeof(struct sockaddr_in);
    // Populating the address with { i, i, i, i }.
    memset(&addr->sin_addr, i, kIPv4AddressSize);
    addr->sin_family = AF_INET;
    // Set port to i << 2;
    addr->sin_port = base::HostToNet16(static_cast<uint16>(i << 2));
    memset(&ai[i], 0x0, sizeof(ai[i]));
    ai[i].ai_family = addr->sin_family;
    ai[i].ai_socktype = SOCK_STREAM;
    ai[i].ai_addrlen = storage[i].addr_len;
    ai[i].ai_addr = storage[i].addr;
    if (i + 1 < kNumElements)
      ai[i].ai_next = &ai[i + 1];
  }

  AddressList list = AddressList::CreateFromAddrinfo(&ai[0]);

  ASSERT_EQ(kNumElements, list.size());
  for (size_t i = 0; i < list.size(); ++i) {
    EXPECT_EQ(ADDRESS_FAMILY_IPV4, list[i].GetFamily());
    // Only check the first byte of the address.
    EXPECT_EQ(i, list[i].address()[0]);
    EXPECT_EQ(static_cast<int>(i << 2), list[i].port());
  }

  // Check if operator= works.
  AddressList copy;
  copy = list;
  ASSERT_EQ(kNumElements, copy.size());

  // Check if copy is independent.
  copy[1] = IPEndPoint(copy[2].address(), 0xBEEF);
  // Original should be unchanged.
  EXPECT_EQ(1u, list[1].address()[0]);
  EXPECT_EQ(1 << 2, list[1].port());
}
#endif  // defined(OS_STARBOARD)

TEST(AddressListTest, CreateFromIPAddressList) {
  struct TestData {
    std::string ip_address;
  } tests[] = {
    { "127.0.0.1" },
#if defined(IN6ADDR_ANY_INIT)
    { "2001:db8:0::42" },
#endif
    { "192.168.1.1" },
  };
  const std::string kCanonicalName = "canonical.example.com";

  // Construct a list of ip addresses.
  IPAddressList ip_list;
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(tests); ++i) {
    IPAddressNumber ip_number;
    ASSERT_TRUE(ParseIPLiteralToNumber(tests[i].ip_address, &ip_number));
    ip_list.push_back(ip_number);
  }

  AddressList test_list = AddressList::CreateFromIPAddressList(ip_list,
                                                               kCanonicalName);
  std::string canonical_name;
  EXPECT_EQ(kCanonicalName, test_list.canonical_name());
  EXPECT_EQ(ARRAYSIZE_UNSAFE(tests), test_list.size());
}

}  // namespace
}  // namespace net
