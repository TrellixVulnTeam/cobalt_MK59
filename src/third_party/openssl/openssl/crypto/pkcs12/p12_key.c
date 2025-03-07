/* p12_key.c */
/*
 * Written by Dr Stephen N Henson (steve@openssl.org) for the OpenSSL project
 * 1999.
 */
/* ====================================================================
 * Copyright (c) 1999 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    licensing@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */

#include <openssl/opensslconf.h>
#if !defined(OPENSSL_SYS_STARBOARD)
#include <stdio.h>
#endif  // !defined(OPENSSL_SYS_STARBOARD)
#include "cryptlib.h"
#include <openssl/pkcs12.h>
#include <openssl/bn.h>

/* Uncomment out this line to get debugging info about key generation */
/*
 * #define DEBUG_KEYGEN
 */
#ifdef DEBUG_KEYGEN
# include <openssl/bio.h>
extern BIO *bio_err;
void h__dump(unsigned char *p, int len);
#endif

/* PKCS12 compatible key/IV generation */
#ifndef min
# define min(a,b) ((a) < (b) ? (a) : (b))
#endif

int PKCS12_key_gen_asc(const char *pass, int passlen, unsigned char *salt,
                       int saltlen, int id, int iter, int n,
                       unsigned char *out, const EVP_MD *md_type)
{
    int ret;
    unsigned char *unipass;
    int uniplen;

    if (!pass) {
        unipass = NULL;
        uniplen = 0;
    } else if (!OPENSSL_asc2uni(pass, passlen, &unipass, &uniplen)) {
        PKCS12err(PKCS12_F_PKCS12_KEY_GEN_ASC, ERR_R_MALLOC_FAILURE);
        return 0;
    }
    ret = PKCS12_key_gen_uni(unipass, uniplen, salt, saltlen,
                             id, iter, n, out, md_type);
    if (ret <= 0)
        return 0;
    if (unipass) {
        OPENSSL_cleanse(unipass, uniplen); /* Clear password from memory */
        OPENSSL_free(unipass);
    }
    return ret;
}

int PKCS12_key_gen_uni(unsigned char *pass, int passlen, unsigned char *salt,
                       int saltlen, int id, int iter, int n,
                       unsigned char *out, const EVP_MD *md_type)
{
    unsigned char *B, *D, *I, *p, *Ai;
    int Slen, Plen, Ilen, Ijlen;
    int i, j, u, v;
    int ret = 0;
    BIGNUM *Ij, *Bpl1;          /* These hold Ij and B + 1 */
    EVP_MD_CTX ctx;
#ifdef  DEBUG_KEYGEN
    unsigned char *tmpout = out;
    int tmpn = n;
#endif

#if 0
    if (!pass) {
        PKCS12err(PKCS12_F_PKCS12_KEY_GEN_UNI, ERR_R_PASSED_NULL_PARAMETER);
        return 0;
    }
#endif

    EVP_MD_CTX_init(&ctx);
#ifdef  DEBUG_KEYGEN
    OPENSSL_port_printferr("KEYGEN DEBUG\n");
    OPENSSL_port_printferr("ID %d, ITER %d\n", id, iter);
    OPENSSL_port_printferr("Password (length %d):\n", passlen);
    h__dump(pass, passlen);
    OPENSSL_port_printferr("Salt (length %d):\n", saltlen);
    h__dump(salt, saltlen);
#endif
    v = EVP_MD_block_size(md_type);
    u = EVP_MD_size(md_type);
    if (u < 0)
        return 0;
    D = OPENSSL_malloc(v);
    Ai = OPENSSL_malloc(u);
    B = OPENSSL_malloc(v + 1);
    Slen = v * ((saltlen + v - 1) / v);
    if (passlen)
        Plen = v * ((passlen + v - 1) / v);
    else
        Plen = 0;
    Ilen = Slen + Plen;
    I = OPENSSL_malloc(Ilen);
    Ij = BN_new();
    Bpl1 = BN_new();
    if (!D || !Ai || !B || !I || !Ij || !Bpl1)
        goto err;
    for (i = 0; i < v; i++)
        D[i] = id;
    p = I;
    for (i = 0; i < Slen; i++)
        *p++ = salt[i % saltlen];
    for (i = 0; i < Plen; i++)
        *p++ = pass[i % passlen];
    for (;;) {
        if (!EVP_DigestInit_ex(&ctx, md_type, NULL)
            || !EVP_DigestUpdate(&ctx, D, v)
            || !EVP_DigestUpdate(&ctx, I, Ilen)
            || !EVP_DigestFinal_ex(&ctx, Ai, NULL))
            goto err;
        for (j = 1; j < iter; j++) {
            if (!EVP_DigestInit_ex(&ctx, md_type, NULL)
                || !EVP_DigestUpdate(&ctx, Ai, u)
                || !EVP_DigestFinal_ex(&ctx, Ai, NULL))
                goto err;
        }
        OPENSSL_port_memcpy(out, Ai, min(n, u));
        if (u >= n) {
#ifdef DEBUG_KEYGEN
            OPENSSL_port_printferr("Output KEY (length %d)\n", tmpn);
            h__dump(tmpout, tmpn);
#endif
            ret = 1;
            goto end;
        }
        n -= u;
        out += u;
        for (j = 0; j < v; j++)
            B[j] = Ai[j % u];
        /* Work out B + 1 first then can use B as tmp space */
        if (!BN_bin2bn(B, v, Bpl1))
            goto err;
        if (!BN_add_word(Bpl1, 1))
            goto err;
        for (j = 0; j < Ilen; j += v) {
            if (!BN_bin2bn(I + j, v, Ij))
                goto err;
            if (!BN_add(Ij, Ij, Bpl1))
                goto err;
            if (!BN_bn2bin(Ij, B))
                goto err;
            Ijlen = BN_num_bytes(Ij);
            /* If more than 2^(v*8) - 1 cut off MSB */
            if (Ijlen > v) {
                if (!BN_bn2bin(Ij, B))
                    goto err;
                OPENSSL_port_memcpy(I + j, B + 1, v);
#ifndef PKCS12_BROKEN_KEYGEN
                /* If less than v bytes pad with zeroes */
            } else if (Ijlen < v) {
                OPENSSL_port_memset(I + j, 0, v - Ijlen);
                if (!BN_bn2bin(Ij, I + j + v - Ijlen))
                    goto err;
#endif
            } else if (!BN_bn2bin(Ij, I + j))
                goto err;
        }
    }

 err:
    PKCS12err(PKCS12_F_PKCS12_KEY_GEN_UNI, ERR_R_MALLOC_FAILURE);

 end:
    OPENSSL_free(Ai);
    OPENSSL_free(B);
    OPENSSL_free(D);
    OPENSSL_free(I);
    BN_free(Ij);
    BN_free(Bpl1);
    EVP_MD_CTX_cleanup(&ctx);
    return ret;
}

#ifdef DEBUG_KEYGEN
void h__dump(unsigned char *p, int len)
{
    for (; len--; p++)
        OPENSSL_port_printferr("%02X", *p);
    OPENSSL_port_printferr("\n");
}
#endif
