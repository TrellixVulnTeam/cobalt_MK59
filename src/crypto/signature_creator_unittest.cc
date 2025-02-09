// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/memory/scoped_ptr.h"
#include "crypto/rsa_private_key.h"
#include "crypto/signature_creator.h"
#include "crypto/signature_verifier.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(SignatureCreatorTest, BasicTest) {
  // Do a verify round trip.
  scoped_ptr<crypto::RSAPrivateKey> key_original(
      crypto::RSAPrivateKey::Create(1024));
  ASSERT_TRUE(key_original.get());

  std::vector<uint8> key_info;
  key_original->ExportPrivateKey(&key_info);
  scoped_ptr<crypto::RSAPrivateKey> key(
      crypto::RSAPrivateKey::CreateFromPrivateKeyInfo(key_info));
  ASSERT_TRUE(key.get());

  scoped_ptr<crypto::SignatureCreator> signer(
      crypto::SignatureCreator::Create(key.get()));
  ASSERT_TRUE(signer.get());

  std::string data("Hello, World!");
  ASSERT_TRUE(signer->Update(reinterpret_cast<const uint8*>(data.c_str()),
                             data.size()));

  std::vector<uint8> signature;
  ASSERT_TRUE(signer->Final(&signature));

  std::vector<uint8> public_key_info;
  ASSERT_TRUE(key_original->ExportPublicKey(&public_key_info));

  crypto::SignatureVerifier verifier;
  ASSERT_TRUE(verifier.VerifyInit(
      crypto::SignatureVerifier::RSA_PKCS1_SHA1, &signature.front(),
      signature.size(), &public_key_info.front(), public_key_info.size()));

  verifier.VerifyUpdate(reinterpret_cast<const uint8*>(data.c_str()),
                        data.size());
  ASSERT_TRUE(verifier.VerifyFinal());
}
