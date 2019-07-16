/****************************************************************/
/* AES-CMAC with AES-128 bit */
/* CMAC Algorithm described in SP800-38B */
/* Author: Junhyuk Song (junhyuk.song@samsung.com) */
/* Jicheol Lee (jicheol.lee@samsung.com) */
/****************************************************************/
#include <stdio.h>
#include "aes_cmac.h"

extern void aes128_encrypt(const unsigned char key[16], const unsigned char input[16],
                           unsigned char output[16]);

/* For CMAC Calculation */
unsigned char const_Rb[16] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x87
};

/* Basic Functions */
void xor_128(unsigned char *a, unsigned char *b, unsigned char *out)
{
    int i;
    for (i = 0; i < 16; i++)
    {
        out[i] = a[i] ^ b[i];
    }
}

/* AES-CMAC Generation Function */
void leftshift_onebit(unsigned char *input, unsigned char *output)
{
    int i;
    unsigned char overflow = 0;
    for (i = 15; i >= 0; i--)
    {
        output[i] = input[i] << 1;
        output[i] |= overflow;
        overflow = (input[i] & 0x80) ? 1 : 0;
    }
    return;
}

void generate_subkey(unsigned char *key, unsigned char *K1, unsigned
                     char *K2)
{
    unsigned char L[16];
    unsigned char Z[16];
    unsigned char tmp[16];
    int i;
    for (i = 0; i < 16; i++) { Z[i] = 0; }
    aes128_encrypt(key, Z, L);
    if ((L[0] & 0x80) == 0)     /* If MSB(L) = 0, then K1 = L << 1 */
    {
        leftshift_onebit(L, K1);
    }
    else     /* Else K1 = ( L << 1 ) (+) Rb */
    {
        leftshift_onebit(L, tmp);
        xor_128(tmp, const_Rb, K1);
    }
    if ((K1[0] & 0x80) == 0)
    {
        leftshift_onebit(K1, K2);
    }
    else
    {
        leftshift_onebit(K1, tmp);
        xor_128(tmp, const_Rb, K2);
    }
    return;
}

void padding(unsigned char *lastb, unsigned char *pad, int length)
{
    int j;
    /* original last block */
    for (j = 0; j < 16; j++)
    {
        if (j < length)
        {
            pad[j] = lastb[j];
        }
        else if (j == length)
        {
            pad[j] = 0x80;
        }
        else
        {
            pad[j] = 0x00;
        }
    }
}

void AES_CMAC(unsigned char *key, unsigned char *input, int length,
              unsigned char *mac)
{
    unsigned char X[16], Y[16], M_last[16], padded[16];
    unsigned char K1[16], K2[16];
    int n, i, flag;
    generate_subkey(key, K1, K2);
    n = (length + 15) / 16; /* n is number of rounds */
    if (n == 0)
    {
        n = 1;
        flag = 0;
    }
    else
    {
        if ((length % 16) == 0)   /* last block is a complete block */
        {
            flag = 1;
        }
        else     /* last block is not complete block */
        {
            flag = 0;
        }
    }
    if (flag)     /* last block is complete block */
    {
        xor_128(&input[16 * (n - 1)], K1, M_last);
    }
    else
    {
        padding(&input[16 * (n - 1)], padded, length % 16);
        xor_128(padded, K2, M_last);
    }
    for (i = 0; i < 16; i++) { X[i] = 0; }
    for (i = 0; i < n - 1; i++)
    {
        xor_128(X, &input[16 * i], Y); /* Y := Mi (+) X */
        aes128_encrypt(key, Y, X); /* X := AES-128(KEY, Y); */
    }
    xor_128(X, M_last, Y);
    aes128_encrypt(key, Y, X);
    for (i = 0; i < 16; i++)
    {
        mac[i] = X[i];
    }
}