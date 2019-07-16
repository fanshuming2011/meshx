/**
 * \file ccm.h
 *
 * \brief This file provides an API for the CCM authenticated encryption
 *        mode for block ciphers.
 *
 * CCM combines Counter mode encryption with CBC-MAC authentication
 * for 128-bit block ciphers.
 *
 * Input to CCM includes the following elements:
 * <ul><li>Payload - data that is both authenticated and encrypted.</li>
 * <li>Associated data (Adata) - data that is authenticated but not
 * encrypted, For example, a header.</li>
 * <li>Nonce - A unique value that is assigned to the payload and the
 * associated data.</li></ul>
 *
 * Definition of CCM:
 * http://csrc.nist.gov/publications/nistpubs/800-38C/SP800-38C_updated-July20_2007.pdf
 * RFC 3610 "Counter with CBC-MAC (CCM)"
 *
 * Related:
 * RFC 5116 "An Interface and Algorithms for Authenticated Encryption"
 *
 * Definition of CCM*:
 * IEEE 802.15.4 - IEEE Standard for Local and metropolitan area networks
 * Integer representation is fixed most-significant-octet-first order and
 * the representation of octets is most-significant-bit-first order. This is
 * consistent with RFC 3610.
 */
/*
 *  Copyright (C) 2006-2018, Arm Limited (or its affiliates), All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of Mbed TLS (https://tls.mbed.org)
 */

#ifndef MBEDTLS_CCM_H
#define MBEDTLS_CCM_H

#include <stdio.h>


/**
 * \brief    The CCM context-type definition. The CCM context is passed
 *           to the APIs called.
 */
typedef struct mbedtls_ccm_context
{
    uint8_t key[16];
} mbedtls_ccm_context;

/**
 * \brief           This function encrypts a buffer using CCM.
 *
 * \note            The tag is written to a separate buffer. To concatenate
 *                  the \p tag with the \p output, as done in <em>RFC-3610:
 *                  Counter with CBC-MAC (CCM)</em>, use
 *                  \p tag = \p output + \p length, and make sure that the
 *                  output buffer is at least \p length + \p tag_len wide.
 *
 * \param ctx       The CCM context to use for encryption. This must be
 *                  initialized and bound to a key.
 * \param length    The length of the input data in Bytes.
 * \param iv        The initialization vector (nonce). This must be a readable
 *                  buffer of at least \p iv_len Bytes.
 * \param iv_len    The length of the nonce in Bytes: 7, 8, 9, 10, 11, 12,
 *                  or 13. The length L of the message length field is
 *                  15 - \p iv_len.
 * \param add       The additional data field. If \p add_len is greater than
 *                  zero, \p add must be a readable buffer of at least that
 *                  length.
 * \param add_len   The length of additional data in Bytes.
 *                  This must be less than `2^16 - 2^8`.
 * \param input     The buffer holding the input data. If \p length is greater
 *                  than zero, \p input must be a readable buffer of at least
 *                  that length.
 * \param output    The buffer holding the output data. If \p length is greater
 *                  than zero, \p output must be a writable buffer of at least
 *                  that length.
 * \param tag       The buffer holding the authentication field. This must be a
 *                  readable buffer of at least \p tag_len Bytes.
 * \param tag_len   The length of the authentication field to generate in Bytes:
 *                  4, 6, 8, 10, 12, 14 or 16.
 *
 * \return          \c 0 on success.
 * \return          A CCM or cipher-specific error code on failure.
 */
int mbedtls_ccm_encrypt_and_tag(mbedtls_ccm_context *ctx, size_t length,
                                const unsigned char *iv, size_t iv_len,
                                const unsigned char *add, size_t add_len,
                                const unsigned char *input, unsigned char *output,
                                unsigned char *tag, size_t tag_len);

/**
 * \brief           This function performs a CCM authenticated decryption of a
 *                  buffer.
 *
 * \param ctx       The CCM context to use for decryption. This must be
 *                  initialized and bound to a key.
 * \param length    The length of the input data in Bytes.
 * \param iv        The initialization vector (nonce). This must be a readable
 *                  buffer of at least \p iv_len Bytes.
 * \param iv_len    The length of the nonce in Bytes: 7, 8, 9, 10, 11, 12,
 *                  or 13. The length L of the message length field is
 *                  15 - \p iv_len.
 * \param add       The additional data field. This must be a readable buffer
 *                  of at least that \p add_len Bytes..
 * \param add_len   The length of additional data in Bytes.
 *                  This must be less than 2^16 - 2^8.
 * \param input     The buffer holding the input data. If \p length is greater
 *                  than zero, \p input must be a readable buffer of at least
 *                  that length.
 * \param output    The buffer holding the output data. If \p length is greater
 *                  than zero, \p output must be a writable buffer of at least
 *                  that length.
 * \param tag       The buffer holding the authentication field. This must be a
 *                  readable buffer of at least \p tag_len Bytes.
 * \param tag_len   The length of the authentication field to generate in Bytes:
 *                  4, 6, 8, 10, 12, 14 or 16.
 *
 * \return          \c 0 on success. This indicates that the message is authentic.
 * \return          #MBEDTLS_ERR_CCM_AUTH_FAILED if the tag does not match.
 * \return          A cipher-specific error code on calculation failure.
 */
int mbedtls_ccm_auth_decrypt(mbedtls_ccm_context *ctx, size_t length,
                             const unsigned char *iv, size_t iv_len,
                             const unsigned char *add, size_t add_len,
                             const unsigned char *input, unsigned char *output,
                             const unsigned char *tag, size_t tag_len);


#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_CCM_H */