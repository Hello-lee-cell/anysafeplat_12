﻿/**
 * \file sm4.h
 * \brief sm4 block cipher
 */

#ifndef BOCALG_SM4_H
#define BOCALG_SM4_H

#include <string.h>

#if defined(_MSC_VER) && !defined(EFIX64) && !defined(EFI32)
#include <basetsd.h>
typedef UINT32 uint32_t;
#else
#include <inttypes.h>
#endif

#define SM4_ENCRYPT     1
#define SM4_DECRYPT     0

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          SM4 context structure
 */
typedef struct
{
    int mode;                   /*!<  encrypt/decrypt   */
    uint32_t sk[32];			/*!<  SM4 subkeys       */
    bool  isPadding;
}
sm4_context;

void sm4_init( sm4_context *ctx );
void sm4_free( sm4_context *ctx );

/**
 * \brief          SM4 key schedule (128-bit, encryption)
 *
 * \param ctx      SM4 context to be initialized
 * \param key      16-byte secret key
 */
void sm4_setkey_enc( sm4_context *ctx, unsigned char key[16] );

/**
 * \brief          SM4 key schedule (128-bit, decryption)
 *
 * \param ctx      SM4 context to be initialized
 * \param key      16-byte secret key
 */
void sm4_setkey_dec( sm4_context *ctx, unsigned char key[16] );

/**
 * \brief          SM4-ECB block encryption/decryption
 * \param ctx      SM4 context
 * \param mode     SM4_ENCRYPT or SM4_DECRYPT
 * \param length   length of the input data
 * \param input    input block
 * \param output   output block
 */
int sm4_crypt_ecb( sm4_context *ctx,
                     unsigned char *input,
                     unsigned char *output);

/**
 * \brief          SM4-CBC buffer encryption/decryption
 * \param ctx      SM4 context
 * \param mode     SM4_ENCRYPT or SM4_DECRYPT
 * \param length   length of the input data
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 */

int sm4_crypt_cbc( sm4_context *ctx,
                     int mode,
                     unsigned char iv[16],
                     unsigned char *input,
                     unsigned char *output );

unsigned char* padding(unsigned char input[1000],int mode);

#ifdef __cplusplus
}
#endif

#endif /* sm4.h */
