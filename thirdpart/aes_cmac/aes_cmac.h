/****************************************************************/
/* AES-CMAC with AES-128 bit */
/* CMAC Algorithm described in SP800-38B */
/* Author: Junhyuk Song (junhyuk.song@samsung.com) */
/* Jicheol Lee (jicheol.lee@samsung.com) */
/****************************************************************/

#ifndef _AES_CMAC_H_
#define _AES_CMAC_H_


extern void AES_CMAC(unsigned char *key, unsigned char *input, int length,
                     unsigned char *mac);


#endif