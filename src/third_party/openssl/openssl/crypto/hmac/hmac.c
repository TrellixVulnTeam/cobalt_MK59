/* crypto/hmac/hmac.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 *
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 *
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 *
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */
#include <openssl/opensslconf.h>
#if !defined(OPENSSL_SYS_STARBOARD)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif  // !defined(OPENSSL_SYS_STARBOARD)
#include "cryptlib.h"
#include <openssl/hmac.h>

#ifdef OPENSSL_FIPS
# include <openssl/fips.h>
#endif

int HMAC_Init_ex(HMAC_CTX *ctx, const void *key, int len,
                 const EVP_MD *md, ENGINE *impl)
{
    int i, j, reset = 0;
    unsigned char pad[HMAC_MAX_MD_CBLOCK];

#ifdef OPENSSL_FIPS
    if (FIPS_mode()) {
        /* If we have an ENGINE need to allow non FIPS */
        if ((impl || ctx->i_ctx.engine)
            && !(ctx->i_ctx.flags & EVP_CIPH_FLAG_NON_FIPS_ALLOW)) {
            EVPerr(EVP_F_HMAC_INIT_EX, EVP_R_DISABLED_FOR_FIPS);
            return 0;
        }
        /*
         * Other algorithm blocking will be done in FIPS_cmac_init, via
         * FIPS_hmac_init_ex().
         */
        if (!impl && !ctx->i_ctx.engine)
            return FIPS_hmac_init_ex(ctx, key, len, md, NULL);
    }
#endif
    /* If we are changing MD then we must have a key */
    if (md != NULL && md != ctx->md && (key == NULL || len < 0))
        return 0;

    if (md != NULL) {
        reset = 1;
        ctx->md = md;
    } else if (ctx->md) {
        md = ctx->md;
    } else {
        return 0;
    }

    if (key != NULL) {
        reset = 1;
        j = EVP_MD_block_size(md);
        OPENSSL_assert(j <= (int)sizeof(ctx->key));
        if (j < len) {
            if (!EVP_DigestInit_ex(&ctx->md_ctx, md, impl))
                goto err;
            if (!EVP_DigestUpdate(&ctx->md_ctx, key, len))
                goto err;
            if (!EVP_DigestFinal_ex(&(ctx->md_ctx), ctx->key,
                                    &ctx->key_length))
                goto err;
        } else {
            if (len < 0 || len > (int)sizeof(ctx->key))
                return 0;
            OPENSSL_port_memcpy(ctx->key, key, len);
            ctx->key_length = len;
        }
        if (ctx->key_length != HMAC_MAX_MD_CBLOCK)
            OPENSSL_port_memset(&ctx->key[ctx->key_length], 0,
                   HMAC_MAX_MD_CBLOCK - ctx->key_length);
    }

    if (reset) {
        for (i = 0; i < HMAC_MAX_MD_CBLOCK; i++)
            pad[i] = 0x36 ^ ctx->key[i];
        if (!EVP_DigestInit_ex(&ctx->i_ctx, md, impl))
            goto err;
        if (!EVP_DigestUpdate(&ctx->i_ctx, pad, EVP_MD_block_size(md)))
            goto err;

        for (i = 0; i < HMAC_MAX_MD_CBLOCK; i++)
            pad[i] = 0x5c ^ ctx->key[i];
        if (!EVP_DigestInit_ex(&ctx->o_ctx, md, impl))
            goto err;
        if (!EVP_DigestUpdate(&ctx->o_ctx, pad, EVP_MD_block_size(md)))
            goto err;
    }
    if (!EVP_MD_CTX_copy_ex(&ctx->md_ctx, &ctx->i_ctx))
        goto err;
    return 1;
 err:
    return 0;
}

int HMAC_Init(HMAC_CTX *ctx, const void *key, int len, const EVP_MD *md)
{
    if (key && md)
        HMAC_CTX_init(ctx);
    return HMAC_Init_ex(ctx, key, len, md, NULL);
}

int HMAC_Update(HMAC_CTX *ctx, const unsigned char *data, size_t len)
{
#ifdef OPENSSL_FIPS
    if (FIPS_mode() && !ctx->i_ctx.engine)
        return FIPS_hmac_update(ctx, data, len);
#endif
    if (!ctx->md)
        return 0;

    return EVP_DigestUpdate(&ctx->md_ctx, data, len);
}

int HMAC_Final(HMAC_CTX *ctx, unsigned char *md, unsigned int *len)
{
    unsigned int i;
    unsigned char buf[EVP_MAX_MD_SIZE];
#ifdef OPENSSL_FIPS
    if (FIPS_mode() && !ctx->i_ctx.engine)
        return FIPS_hmac_final(ctx, md, len);
#endif

    if (!ctx->md)
        goto err;

    if (!EVP_DigestFinal_ex(&ctx->md_ctx, buf, &i))
        goto err;
    if (!EVP_MD_CTX_copy_ex(&ctx->md_ctx, &ctx->o_ctx))
        goto err;
    if (!EVP_DigestUpdate(&ctx->md_ctx, buf, i))
        goto err;
    if (!EVP_DigestFinal_ex(&ctx->md_ctx, md, len))
        goto err;
    return 1;
 err:
    return 0;
}

void HMAC_CTX_init(HMAC_CTX *ctx)
{
    EVP_MD_CTX_init(&ctx->i_ctx);
    EVP_MD_CTX_init(&ctx->o_ctx);
    EVP_MD_CTX_init(&ctx->md_ctx);
    ctx->md = NULL;
}

int HMAC_CTX_copy(HMAC_CTX *dctx, HMAC_CTX *sctx)
{
    if (!EVP_MD_CTX_copy(&dctx->i_ctx, &sctx->i_ctx))
        goto err;
    if (!EVP_MD_CTX_copy(&dctx->o_ctx, &sctx->o_ctx))
        goto err;
    if (!EVP_MD_CTX_copy(&dctx->md_ctx, &sctx->md_ctx))
        goto err;
    OPENSSL_port_memcpy(dctx->key, sctx->key, HMAC_MAX_MD_CBLOCK);
    dctx->key_length = sctx->key_length;
    dctx->md = sctx->md;
    return 1;
 err:
    return 0;
}

void HMAC_CTX_cleanup(HMAC_CTX *ctx)
{
#ifdef OPENSSL_FIPS
    if (FIPS_mode() && !ctx->i_ctx.engine) {
        FIPS_hmac_ctx_cleanup(ctx);
        return;
    }
#endif
    EVP_MD_CTX_cleanup(&ctx->i_ctx);
    EVP_MD_CTX_cleanup(&ctx->o_ctx);
    EVP_MD_CTX_cleanup(&ctx->md_ctx);
    OPENSSL_port_memset(ctx, 0, sizeof *ctx);
}

unsigned char *HMAC(const EVP_MD *evp_md, const void *key, int key_len,
                    const unsigned char *d, size_t n, unsigned char *md,
                    unsigned int *md_len)
{
    HMAC_CTX c;
    static unsigned char m[EVP_MAX_MD_SIZE];

    if (md == NULL)
        md = m;
    HMAC_CTX_init(&c);
    if (!HMAC_Init(&c, key, key_len, evp_md))
        goto err;
    if (!HMAC_Update(&c, d, n))
        goto err;
    if (!HMAC_Final(&c, md, md_len))
        goto err;
    HMAC_CTX_cleanup(&c);
    return md;
 err:
    HMAC_CTX_cleanup(&c);
    return NULL;
}

void HMAC_CTX_set_flags(HMAC_CTX *ctx, unsigned long flags)
{
    EVP_MD_CTX_set_flags(&ctx->i_ctx, flags);
    EVP_MD_CTX_set_flags(&ctx->o_ctx, flags);
    EVP_MD_CTX_set_flags(&ctx->md_ctx, flags);
}
