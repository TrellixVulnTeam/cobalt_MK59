/* crypto/cpt_err.c */
/* ====================================================================
 * Copyright (c) 1999-2011 The OpenSSL Project.  All rights reserved.
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
 *    openssl-core@OpenSSL.org.
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

/*
 * NOTE: this file was auto generated by the mkerr.pl script: any changes
 * made to it will be overwritten when the script next updates this file,
 * only reason strings will be preserved.
 */

#include <openssl/opensslconf.h>
#if !defined(OPENSSL_SYS_STARBOARD)
#include <stdio.h>
#endif  // !defined(OPENSSL_SYS_STARBOARD)
#include <openssl/err.h>
#include <openssl/crypto.h>

/* BEGIN ERROR CODES */
#ifndef OPENSSL_NO_ERR

# define ERR_FUNC(func) ERR_PACK(ERR_LIB_CRYPTO,func,0)
# define ERR_REASON(reason) ERR_PACK(ERR_LIB_CRYPTO,0,reason)

static ERR_STRING_DATA CRYPTO_str_functs[] = {
    {ERR_FUNC(CRYPTO_F_CRYPTO_GET_EX_NEW_INDEX), "CRYPTO_get_ex_new_index"},
    {ERR_FUNC(CRYPTO_F_CRYPTO_GET_NEW_DYNLOCKID), "CRYPTO_get_new_dynlockid"},
    {ERR_FUNC(CRYPTO_F_CRYPTO_GET_NEW_LOCKID), "CRYPTO_get_new_lockid"},
    {ERR_FUNC(CRYPTO_F_CRYPTO_SET_EX_DATA), "CRYPTO_set_ex_data"},
    {ERR_FUNC(CRYPTO_F_DEF_ADD_INDEX), "DEF_ADD_INDEX"},
    {ERR_FUNC(CRYPTO_F_DEF_GET_CLASS), "DEF_GET_CLASS"},
    {ERR_FUNC(CRYPTO_F_FIPS_MODE_SET), "FIPS_mode_set"},
    {ERR_FUNC(CRYPTO_F_INT_DUP_EX_DATA), "INT_DUP_EX_DATA"},
    {ERR_FUNC(CRYPTO_F_INT_FREE_EX_DATA), "INT_FREE_EX_DATA"},
    {ERR_FUNC(CRYPTO_F_INT_NEW_EX_DATA), "INT_NEW_EX_DATA"},
    {0, NULL}
};

static ERR_STRING_DATA CRYPTO_str_reasons[] = {
    {ERR_REASON(CRYPTO_R_FIPS_MODE_NOT_SUPPORTED), "fips mode not supported"},
    {ERR_REASON(CRYPTO_R_NO_DYNLOCK_CREATE_CALLBACK),
     "no dynlock create callback"},
    {0, NULL}
};

#endif

void ERR_load_CRYPTO_strings(void)
{
#ifndef OPENSSL_NO_ERR

    if (ERR_func_error_string(CRYPTO_str_functs[0].error) == NULL) {
        ERR_load_strings(0, CRYPTO_str_functs);
        ERR_load_strings(0, CRYPTO_str_reasons);
    }
#endif
}
