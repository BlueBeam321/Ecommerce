/**
* sj_crypt.c
*
* Copyright (c) 2016 - 2018 Code
*
* Description:
*          SinJong Data Crypt Module
*
* Authors:
*          Rim Myong Su
*
* History:
*          Created by RMS on 07/12/2018
*/

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define OPEN_SSL_USE 0
#define AES_ENCRYPTION_BLOCK_SIZE 1024
#define AES_DECRYPTION_BLOCK_SIZE 1040

#if OPEN_SSL_USE
#include <openssl/rsa.h>
#include <openssl/md5.h>
#else
#include "rsa/bigd.h"
#endif

#include "aes/aes.h"
#include "crypt.h"
static uint8_t _TextKey[512] = { 0 };

static inline void  _OctTo36(char *oct, int size, char*threeth);
static uint8_t InitKeyEngine();

#define AES_KEY_SIZE 256
//AES_IV
static unsigned char AES_IV[16] = { 0xf0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

const unsigned char crypt_rep_table[80] = {
    0x0C, 0x13, 0xE2, 0xD6, 0x62, 0xE1, 0x39, 0x50, 0xE0, 0x58, 0x6F, 0x06, 0xC6, 0xD6, 0x15, 0x6D,
    0xEE, 0xFC, 0x3B, 0x72, 0x5D, 0xF8, 0x03, 0x79, 0x87, 0x78, 0x89, 0xC7, 0x7F, 0xBE, 0x8F, 0x00,
    0x89, 0x86, 0x1B, 0xC6, 0x88, 0x04, 0x5B, 0x3B, 0x5E, 0xA0, 0xFF, 0x8C, 0x67, 0xF4, 0xB4, 0xCB,
    0xA0, 0xD6, 0x16, 0x58, 0x98, 0x68, 0x6E, 0x53, 0x88, 0x93, 0x29, 0xAD, 0xBE, 0x63, 0x85, 0x97,
    0xE2, 0xAF, 0x01, 0xAA, 0x86, 0xBE, 0xC7, 0xE5, 0x72, 0x77, 0x41, 0x9E, 0x2E, 0x68, 0x49, 0x93
};

const uint8_t _36_rep_table[48] = {
    0x70, 0xB4, 0x6B, 0x78, 0x4C, 0x69, 0x1B, 0xA5, 0x5F, 0x59, 0xC3, 0x0F, 0x77, 0x7E, 0x32, 0xCC,
    0xC1, 0xFE, 0x77, 0x4E, 0x31, 0x2B, 0x2E, 0x62, 0x4E, 0x74, 0xDF, 0x60, 0xA2, 0x75, 0xCF, 0x43,
    0x61, 0x46, 0xE7, 0x8C, 0xA1, 0xB3, 0x30, 0xD7, 0x79, 0x8C, 0xD3, 0x72, 0xDD, 0x35, 0x19, 0x6D
};

uint8_t sj_rep_tlb[128] = "";
uint8_t sj_63_rep_table[128] = "";

uint8_t sinjong_n[1024] ="";
#if SINJONG_SERVER
uint8_t sinjong_e[1024] = "";
#endif
uint8_t sinjong_d[1024] = "";


static __inline int _8b26b(uint8_t *src/*size = 3*/, uint8_t size, uint8_t *dst/*size = 4*/) {

    if (size == 1) {
        dst[0] = src[0] >> 2 & 0xff;
        dst[1] = (src[0] & 3) << 4;
        dst[0] = sj_rep_tlb[dst[0]];
        dst[1] = sj_rep_tlb[dst[1]];
        return 2;
    }

    if (size == 2) {
        dst[0] = src[0] >> 2 & 0xff;
        dst[1] = (src[0] & 0x03) << 4 | (src[1] >> 4 & 0x0f);
        dst[2] = (src[1] & 0x0f) << 2;

        dst[0] = sj_rep_tlb[dst[0]];
        dst[1] = sj_rep_tlb[dst[1]];
        dst[2] = sj_rep_tlb[dst[2]];
        return 3;
    }

    if (size == 3) {

        dst[0] = src[0] >> 2 & 0xff;
        dst[1] = (src[0] & 0x03) << 4 | (src[1] >> 4 & 0x0f);
        dst[2] = (src[1] & 0x0f) << 2 | (src[2] >> 6 & 0x03);
        dst[3] = src[2] & 0x3f;

        dst[0] = sj_rep_tlb[dst[0]];
        dst[1] = sj_rep_tlb[dst[1]];
        dst[2] = sj_rep_tlb[dst[2]];
        dst[3] = sj_rep_tlb[dst[3]];
        return 4;
    }

    return -1;
}

static __inline int _6b28b(uint8_t *src/*size = 4*/, uint8_t size, uint8_t *dst/*size = 3*/) {
    if (size == 2) {
        src[0] = (uint8_t*)strchr(sj_rep_tlb, src[0]) - sj_rep_tlb;
        src[1] = (uint8_t*)strchr(sj_rep_tlb, src[1]) - sj_rep_tlb;

        dst[0] = (src[0] << 2 & 0xff) | (src[1] >> 4 & 0xff);

        return 1;
    }
    if (size == 3) {
        src[0] = (uint8_t*)strchr(sj_rep_tlb, src[0]) - sj_rep_tlb;
        src[1] = (uint8_t*)strchr(sj_rep_tlb, src[1]) - sj_rep_tlb;
        src[2] = (uint8_t*)strchr(sj_rep_tlb, src[2]) - sj_rep_tlb;

        dst[0] = (src[0] << 2 & 0xff) | (src[1] >> 4 & 0xff);
        dst[1] = (src[1] << 4 & 0xff) | (src[2] >> 2 & 0xff);

        return 2;
    }
    if (size == 4) {
        src[0] = (uint8_t*)strchr(sj_rep_tlb, src[0]) - sj_rep_tlb;
        src[1] = (uint8_t*)strchr(sj_rep_tlb, src[1]) - sj_rep_tlb;
        src[2] = (uint8_t*)strchr(sj_rep_tlb, src[2]) - sj_rep_tlb;
        src[3] = (uint8_t*)strchr(sj_rep_tlb, src[3]) - sj_rep_tlb;

        dst[0] = (src[0] << 2 & 0xff) | (src[1] >> 4 & 0xff);
        dst[1] = (src[1] << 4 & 0xff) | (src[2] >> 2 & 0xff);
        dst[2] = (src[2] & 0x03) << 6 | (src[3] & 0x3f);
        
        return 3;
    }

    return 0;

}

static __inline sj_key_t* bin2char(sj_key_t *binary) {
    
    sj_key_t *text = (sj_key_t*)malloc(sizeof(sj_key_t));
    int _8blen, _6blen, i;
    uint8_t *dst, *src;

    src = binary->data;
    _8blen = binary->size;

    _6blen = 8 * _8blen / 6;

    if (_8blen * 8 % 6) //Not match length 1->2 2->3
    {
        _6blen++;
    }
    dst = (uint8_t *)malloc(_6blen);
    memset(dst, 0, _6blen);

    int loop = _8blen / 3;
    for (i = 0; i < loop; i++)
    {
        _8b26b(src + (i * 3), 3, dst + (i * 4));
    }

    if (_8blen % 3)
    {
        _8b26b(src + loop * 3, _8blen - loop * 3, dst + loop * 4);
    }
    
    text->data = dst;
    text->size = _6blen;

    return text;

}

static __inline sj_key_t* char2bin(sj_key_t *text) {

    sj_key_t *binary = (sj_key_t*)malloc(sizeof(sj_key_t));

    int _8blen, _6blen, loop = 0, i;
    uint8_t *dst, *src;
    
    _6blen = text->size;
    src = text->data;
    _8blen = 6 * _6blen / 8;

    dst = (uint8_t*)malloc(_8blen);
    memset(dst, 0, _8blen);

    loop = _6blen / 4;
    for (i = 0; i < loop; i++)
    {
        _6b28b(src + (i * 4), 4, dst + (i * 3));
    }

    if (_6blen % 4)
    {
        _6b28b(src + loop * 4, _6blen - loop * 4, dst + loop * 3);
    }

    binary->data = dst;
    binary->size = _8blen;
    return binary;
}

#if SINJONG_SERVER
sj_key_t* sinjong_aes_encrypt(IN sj_key_t* plain, IN uint8_t* key) {

    {
        int len = plain->size;
        unsigned int rest = len % AES_BLOCK_SIZE;
        unsigned int padding = (AES_BLOCK_SIZE - rest);
        if (rest == 0)
        {
            padding = 0;
        }
        unsigned int cipher_length = len + padding;

        unsigned char *input = (unsigned char *)malloc(cipher_length);
        memset(input, 0, cipher_length);
        memcpy(input, plain->data, len);

        if (padding > 0) {
            memset(input + len, (unsigned char)padding, padding);
        }

        unsigned char * buff = (unsigned char*)malloc(cipher_length);
        if (!buff) {
            free(input);
            return NULL;
        }
        memset(buff, cipher_length, 0);

        //set key & iv
        unsigned int key_schedule[AES_BLOCK_SIZE * 4] = { 0 }; //>=53(64)
        aes_key_setup(key, key_schedule, AES_KEY_SIZE);

        aes_encrypt_cbc(input, cipher_length, buff, key_schedule, AES_KEY_SIZE,
            AES_IV);

        sj_key_t *new_key = (sj_key_t*)malloc(sizeof(sj_key_t));
        new_key->data = buff;
        new_key->size = cipher_length;
        
        return new_key;
    }
}
#endif

sj_key_t* sinjong_aes_decrypt(IN sj_key_t* cipher, IN uint8_t* key) {
    
    int len = cipher->size;
    unsigned int rest_len = len % AES_BLOCK_SIZE;
    unsigned int padding_len = 0;
    unsigned int plain_length = len + padding_len;


    unsigned char *input = (unsigned char *)malloc(plain_length + 1);
    memset(input, 0, plain_length);
    memcpy(input, cipher->data, len);

    if (padding_len > 0) {
        memset(input + len, (unsigned char)padding_len, padding_len);
    }

    unsigned char * buff = (unsigned char*)malloc(plain_length);
    if (!buff) {
        free(input);
        return NULL;
    }
    memset(buff, 0, plain_length);

    //set key & iv
    unsigned int key_schedule[AES_BLOCK_SIZE * 4] = { 0 }; //>=53(64)
    aes_key_setup(key, key_schedule, AES_KEY_SIZE);

    aes_decrypt_cbc(input, plain_length, buff, key_schedule, AES_KEY_SIZE,
        AES_IV);

    unsigned char * ptr = buff;
    ptr += (plain_length - 1);
    padding_len = (unsigned int)*ptr;
    if (padding_len > 0 && padding_len <= AES_BLOCK_SIZE) {
        plain_length -= padding_len;
    }
    ptr = NULL;

    sj_key_t *new_key = (sj_key_t*)malloc(sizeof(sj_key_t));
    new_key->data = buff;
    new_key->size = plain_length;
    buff[plain_length] = 0;

    return new_key;
}

#if SINJONG_SERVER && _WIN32 && OPEN_SSL_USE
void sinjong_generate_rsa_key(int bits, uint8_t*e) {

    RSA *rsa = RSA_new();
    BIGNUM *bg_e = BN_new();
    BIGNUM *bg_n = BN_new();
    BIGNUM *bg_p = BN_new();
    BIGNUM *bg_q = BN_new();
    BIGNUM *bg_d = BN_new();
    BIGNUM *bg_ee = BN_new();
    bg_e = BN_bin2bn(e, strlen(e) - 1, NULL);

    RSA_generate_key_ex(rsa, bits, bg_e, NULL);

    RSA_get0_factors(rsa, (const BIGNUM**)&bg_p, (const BIGNUM**)&bg_q);
    RSA_get0_key(rsa, (const BIGNUM**)&bg_n, (const BIGNUM**)&bg_ee, (const BIGNUM**)&bg_d);
    printf("===p===%s\n", BN_bn2dec(bg_p));
    printf("===q===%s\n", BN_bn2dec(bg_q));
    printf("===n===%s\n", BN_bn2dec(bg_n));
    printf("===n bits %d =\n", BN_num_bits(bg_n));
    printf("===ee===%s\n", BN_bn2dec(bg_ee));
    printf("===d===%s\n", BN_bn2dec(bg_d));
    char key_path[256];
    char file_name[256];

    SYSTEMTIME cur_time;
    GetSystemTime(&cur_time);
    sprintf(file_name, "%04d%02d%02d_%02d%02d%02d", cur_time.wYear, cur_time.wMonth, cur_time.wDay, (cur_time.wHour + 9) % 24, cur_time.wMinute, cur_time.wSecond);
    sprintf(key_path, "%s%s", SINJONG_RSA_KEY_PATH, file_name);
    FILE *fp = fopen(key_path, "wb");
    fprintf(fp, "===p===%s\n", BN_bn2dec(bg_p));
    fprintf(fp, "===q===%s\n", BN_bn2dec(bg_q));
    fprintf(fp, "===n===%s\n", BN_bn2dec(bg_n));
    fprintf(fp, "===n bits = %d\n", BN_num_bits(bg_n));
    fprintf(fp, "===ee===%s\n", BN_bn2dec(bg_ee));
    fprintf(fp, "===d===%s\n", BN_bn2dec(bg_d));

    fclose(fp);
}
#endif

#if SINJONG_SERVER
sj_key_t* sinjong_rsa_encrypt(IN sj_key_t *plain) {
    
    sj_key_t *new_key = 0;
    int ret;
    uint8_t *ctext = (uint8_t*)malloc(256);

#if OPEN_SSL_USE
    RSA *key = RSA_new();

    BIGNUM *bg_e = BN_new();
    BIGNUM *bg_n = BN_new();
    BIGNUM *bg_d = BN_new();



    BN_dec2bn(&bg_n, sinjong_n);
    BN_dec2bn(&bg_e, sinjong_e);
    BN_dec2bn(&bg_d, sinjong_d);

    RSA_set0_key(key, bg_n, bg_e, bg_d);

    ret = RSA_private_encrypt(plain->size, plain->data, ctext, key,
        RSA_PKCS1_PADDING);
    if (ret <= 0)
        goto end;
    RSA_free(key);

#else
    BIGD bg_e = bdNew();
    BIGD bg_n = bdNew();
    BIGD bg_d = bdNew();
    BIGD bg_c = bdNew();
    BIGD bg_m = bdNew();


    bdConvFromDecimal(bg_n, sinjong_n);
    bdConvFromDecimal(bg_e, sinjong_e);
    bdConvFromDecimal(bg_d, sinjong_d);

    bdConvFromOctets(bg_m, plain->data, plain->size);

    /* Encrypt c = m^e mod n */
    bdModExp(bg_c, bg_m, bg_e, bg_n);

    ret = bdByteLength(bg_c);

    bdConvToOctets(bg_c, ctext, ret);

    bdFree(&bg_n);
    bdFree(&bg_e);
    bdFree(&bg_d);
    bdFree(&bg_m);
    bdFree(&bg_c);

#endif
    new_key = (sj_key_t*)malloc(sizeof(sj_key_t));
    new_key->data = ctext;
    new_key->size = ret;

    //sj_key_t *final = bin2char(new_key);

end:
    return new_key;
}
#endif

sj_key_t* sinjong_rsa_decrypt(IN sj_key_t *cipher) {

    sj_key_t *new_key = 0;
    int ret;
    uint8_t *ptext = (uint8_t*)malloc(1024);

//    cipher = char2bin(cipher);

#if OPEN_SSL_USE
    RSA *key = RSA_new();

    BIGNUM *bg_e = BN_new();
    BIGNUM *bg_n = BN_new();
    BIGNUM *bg_d = BN_new();



    BN_dec2bn(&bg_n, sinjong_n);
    BN_dec2bn(&bg_e, sinjong_e);

    RSA_set0_key(key, bg_n, bg_e, 0);


    ret = RSA_public_decrypt(cipher->size, cipher->data, ptext, key,
        RSA_PKCS1_PADDING);
    if (ret <= 0)
        goto end;
    RSA_free(key);

#else
    BIGD bg_n = bdNew();
    BIGD bg_d = bdNew();
    BIGD bg_c = bdNew();
    BIGD bg_m = bdNew();


    bdConvFromDecimal(bg_n, sinjong_n);
    bdConvFromDecimal(bg_d, sinjong_d);

    bdConvFromOctets(bg_c, cipher->data, cipher->size);

    /* Encrypt c = m^e mod n */
    /* Decrypt m = c^d mod n */
    bdModExp(bg_m, bg_c, bg_d, bg_n);

    ret = bdByteLength(bg_m);

    bdConvToOctets(bg_m, ptext, ret);

    bdFree(&bg_n);
    bdFree(&bg_d);
    bdFree(&bg_m);
    bdFree(&bg_c);
#endif
    ptext[ret] = 0;
    new_key = (sj_key_t*)malloc(sizeof(sj_key_t));
    new_key->data = ptext;
    new_key->size = ret;
end:
    return new_key;
}

#if SINJONG_SERVER_TMP
sj_key_t* sinjong_generate_license(IN uint8_t* dic_data_key /*size = 24byte*/,
                                                 IN uint8_t* limit_time /*size = 3byte*/,
                                                 IN uint8_t* machine_id /*size = 8byte*/) {
    
    uint8_t aes_key[32] = { 0 };
    uint8_t temp[32] = { 0 };
    sj_key_t *mid_key, *final_key, input_key;

    strcpy(temp, dic_data_key);
    strcat(temp + 24, limit_time);
    strcpy(aes_key, machine_id);

    input_key.data = temp;
    input_key.size = 32;

    mid_key = sinjong_aes_encrypt(&input_key, aes_key);
    final_key = sinjong_rsa_encrypt(mid_key);

    free(mid_key->data);
    free(mid_key);

    final_key->data[final_key->size] = 0;
    return final_key;
}
#endif
#if SINJONG_SERVER
static int my_rand(unsigned char *bytes, size_t nbytes, const unsigned char *seed, size_t seedlen)
/* Our own (very insecure) random generator func using good old rand()
but in the required format for BD_RANDFUNC
-- replace this in practice with your own cryptographically-secure function.
*/
{
    unsigned int myseed;
    size_t i;
    int offset;

    /* Use time for 32-bit seed - then blend in user-supplied seed, if any */
    myseed = (unsigned)time(NULL);
    if (seed)
    {
        for (offset = 0, i = 0; i < seedlen; i++, offset = (offset + 1) % sizeof(unsigned))
            myseed ^= ((unsigned int)seed[i] << (offset * 8));
    }

    srand(myseed);
    while (nbytes--)
    {
        *bytes++ = rand() & 0xFF;
    }

    return 0;
}


static void pr_msg(char *msg, BIGD b)
{
#if 0
    printf("%s", msg);
    bdPrint(b, 1);
#endif
}

static bdigit_t SMALL_PRIMES[] = {
    3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43,
    47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101,
    103, 107, 109, 113,
    127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
    179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
    233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
    283, 293, 307, 311, 313, 317, 331, 337, 347, 349,
    353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
    419, 421, 431, 433, 439, 443, 449, 457, 461, 463,
    467, 479, 487, 491, 499, 503, 509, 521, 523, 541,
    547, 557, 563, 569, 571, 577, 587, 593, 599, 601,
    607, 613, 617, 619, 631, 641, 643, 647, 653, 659,
    661, 673, 677, 683, 691, 701, 709, 719, 727, 733,
    739, 743, 751, 757, 761, 769, 773, 787, 797, 809,
    811, 821, 823, 827, 829, 839, 853, 857, 859, 863,
    877, 881, 883, 887, 907, 911, 919, 929, 937, 941,
    947, 953, 967, 971, 977, 983, 991, 997,
};
#define N_SMALL_PRIMES (sizeof(SMALL_PRIMES)/sizeof(bdigit_t))

static int inline generateRSAPrime(BIGD p, size_t nbits, bdigit_t e, size_t ntests,
    const unsigned char *seed, size_t seedlen, BD_RANDFUNC randFunc)
    /* Create a prime p such that gcd(p-1, e) = 1.
    Returns # prime tests carried out or -1 if failed.
    Sets the TWO highest bits to ensure that the
    product pq will always have its high bit set.
    e MUST be a prime > 2.
    This function assumes that e is prime so we can
    do the less expensive test p mod e != 1 instead
    of gcd(p-1, e) == 1.
    Uses improvement in trial division from Menezes 4.51.
    */
{
    BIGD u;
    size_t i, j, iloop, maxloops, maxodd;
    int done, overflow, failedtrial;
    int count = 0;
    bdigit_t r[N_SMALL_PRIMES];

    /* Create a temp */
    u = bdNew();

    maxodd = nbits * 100;
    maxloops = 5;

    done = 0;
    for (iloop = 0; !done && iloop < maxloops; iloop++)
    {
        /* Set candidate n0 as random odd number */
        bdRandomSeeded(p, nbits, seed, seedlen, randFunc);
        /* Set two highest and low bits */
        bdSetBit(p, nbits - 1, 1);
        bdSetBit(p, nbits - 2, 1);
        bdSetBit(p, 0, 1);

        /* To improve trial division, compute table R[q] = n0 mod q
        for each odd prime q <= B
        */
        for (i = 0; i < N_SMALL_PRIMES; i++)
        {
            r[i] = bdShortMod(u, p, SMALL_PRIMES[i]);
        }

        done = overflow = 0;
        /* Try every odd number n0, n0+2, n0+4,... until we succeed */
        for (j = 0; j < maxodd; j++, overflow = bdShortAdd(p, p, 2))
        {
            /* Check for overflow */
            if (overflow)
                break;

            //give_a_sign('.');
            count++;

            /* Each time 2 is added to the current candidate
            update table R[q] = (R[q] + 2) mod q */
            if (j > 0)
            {
                for (i = 0; i < N_SMALL_PRIMES; i++)
                {
                    r[i] = (r[i] + 2) % SMALL_PRIMES[i];
                }
            }

            /* Candidate passes the trial division stage if and only if
            NONE of the R[q] values equal zero */
            for (failedtrial = 0, i = 0; i < N_SMALL_PRIMES; i++)
            {
                if (r[i] == 0)
                {
                    failedtrial = 1;
                    break;
                }
            }
            if (failedtrial)
                continue;

            /* If p mod e = 1 then gcd(p, e) > 1, so try again */
            bdShortMod(u, p, e);
            if (bdShortCmp(u, 1) == 0)
                continue;

            /* Do expensive primality test */
            //give_a_sign('*');
            if (bdRabinMiller(p, ntests))
            {	/* Success! - we have a prime */
                done = 1;
                break;
            }

        }
    }


    /* Clear up */
    bdFree(&u);
    printf("\n");

    return (done ? count : -1);
}

static inline int generateRSAKey(BIGD n, BIGD e, BIGD d, BIGD p, BIGD q, size_t nbits, bdigit_t ee,  unsigned char *seed, size_t seedlen,
    BD_RANDFUNC randFunc)
{
    BIGD g, p1, q1, phi;
    size_t p_bitsize, q_bitsize;
    size_t ntests = 50;
    unsigned char *myseed = NULL;
    clock_t start, finish;
    double duration, tmake;
    long ptests;
    int res;

    /* Initialise */
    g = bdNew();
    p1 = bdNew();
    q1 = bdNew();
    phi = bdNew();

    myseed = malloc(seedlen + 1);
    if (!myseed) return -1;
    memcpy(myseed, seed, seedlen);

    /* Do (p, q) in two halves, approx equal */
    q_bitsize = nbits / 2;
    p_bitsize = nbits - q_bitsize;

    /* Make sure seeds are slightly different for p and q */
    myseed[seedlen] = 0x01;
    res = generateRSAPrime(p, p_bitsize, ee, ntests, myseed, seedlen + 1, randFunc);
    myseed[seedlen] = 0xff;
    res = generateRSAPrime(q, q_bitsize, ee, ntests, myseed, seedlen + 1, randFunc);

    bdSetShort(e, ee);
    pr_msg("e=", e);

    /* If q > p swap p and q so p > q */
    if (bdCompare(p, q) < 1)
    {
        bdSetEqual(g, p);
        bdSetEqual(p, q);
        bdSetEqual(q, g);
    }

    /* Calc p-1 and q-1 */
    bdSetEqual(p1, p);
    bdDecrement(p1);

    bdSetEqual(q1, q);
    bdDecrement(q1);

    /* Check gcd(p-1, e) = 1 */
    bdGcd(g, p1, e);
    if (bdShortCmp(g, 1) != 0)
        return -1;

    bdGcd(g, q1, e);
    if (bdShortCmp(g, 1) != 0)
        return -1;

    /* Compute n = pq */
    bdMultiply(n, p, q);

    /* Compute d = e^-1 mod (p-1)(q-1) */
    bdMultiply(phi, p1, q1);
    pr_msg("phi=\n", phi);
    res = bdModInv(d, e, phi);
    
    if (res != 0)
        return -1;

    /* Check ed = 1 mod phi */
    bdModMult(g, e, d, phi);

    if (bdShortCmp(g, 1) != 0)
        return -1;

    /* Clean up */
    if (myseed) free(myseed);

    bdFree(&g);
    bdFree(&p1);
    bdFree(&q1);
    bdFree(&phi);

    return 0;
}

#endif

sj_key_t* sinjong_parser_license_key(IN uint8_t* message,
                                               IN uint8_t* machine_id /*size = 8byte*/) {
    uint8_t aes_key[32] = { 0 };
    uint8_t temp[32] = { 0 };
    sj_key_t *mid_key, *final_key, input_key;

    strcpy(aes_key, machine_id);
    input_key.data = message;
    input_key.size = strlen(message);

    mid_key = sinjong_rsa_decrypt(&input_key);
    final_key = sinjong_aes_decrypt(mid_key, aes_key);

    free(mid_key->data);
    free(mid_key);
    final_key->data[final_key->size] = 0;
    return final_key;
}


uint8_t* sinjong_generate_machine_id(IN uint8_t*data) {
    sj_key_t input, *out;
    uint8_t md[16] = { 0 };
    uint8_t*result;
    uint8_t machine_id[10] = { 0 };
    int i, size;
    char aes_key[256] = "19921210";

    if (!sinjong_n[0])
        InitKeyEngine();
    
    memset(machine_id, 0, 10);
    
    for (i = 0; i < strlen(data); i++)
        md[i % 16] ^= data[i];

    input.data = md;
    input.size = 16;

    out = sinjong_aes_decrypt(&input, aes_key);

    for (i = 0; i < 8; i++)
        machine_id[i] = (out->data[i] + out->data[i + 1]) & 0xff;

    machine_id[0] = data[0];
    for (i = 0; i < 8; i++)
        machine_id[8] ^= machine_id[i];
    
    machine_id[9] = 0x40;
    result = malloc(256);
    memset(result, 0, 256);
    
    _OctTo36(machine_id, 10, result);
    return result;
}

#if SINJONG_SERVER

void inline make_text_key(char *dst, char*buff)
{
    sj_key_t *tmp, plain, sj1;
    int len;

    len = strlen(buff);
    plain.size = len;
    plain.data = buff;
 
    tmp = bin2char(&plain);
    memcpy(dst, tmp->data, tmp->size);
    dst[tmp->size] = 0;
    free(tmp->data);
    free(tmp);
}

uint8_t sinjong_make_rsa_key(int nbits, char *pubkey, char *privkey)
{
    unsigned int ee = 257;
    size_t ntests = 50;
    unsigned char *seed = NULL;
    size_t seedlen = 0;
    int ebasic = 0xab3269;
    BIGD n, e, d, p, q;
    BIGD m, c, s, hq, h, m1, m2;
    int res;
    clock_t start, finish;
    double tinv, tcrt;
    char buff[256];
    char tmp[256] = { 0 };

    p = bdNew();
    q = bdNew();
    n = bdNew();
    e = bdNew();
    d = bdNew();
    m = bdNew();
    c = bdNew();
    s = bdNew();
    m1 = bdNew();
    m2 = bdNew();
    h = bdNew();
    hq = bdNew();

    nbits = nbits / 2;
    /* Create RSA key pair (n, e),(d, p, q, dP, dQ, qInv) */
    /* NB we use simple my_rand() here -- you should use a proper cryptographically-secure RNG */

    do 
    {
        res = generateRSAKey(n, e, d, p, q, nbits, ebasic, seed, seedlen, my_rand);
        ebasic++;
    } while (res < 0);

    
    bdConvToDecimal(n, buff, sizeof(buff));
    strcat(tmp, buff);
    strcat(tmp, ":");
    bdConvToDecimal(e, buff, sizeof(buff));
    strcat(tmp, buff);
    make_text_key(privkey, tmp);

    memset(tmp, 0, sizeof(tmp));
    bdConvToDecimal(d, buff, sizeof(buff));
    strcat(tmp, buff);
    strcat(tmp, ":");
    bdConvToDecimal(n, buff, sizeof(buff));
    strcat(tmp, buff);
    make_text_key(pubkey, tmp);

    bdFree(&n);
    bdFree(&e);
    bdFree(&d);
    bdFree(&p);
    bdFree(&q);
    bdFree(&m);
    bdFree(&c);
    bdFree(&s);
    bdFree(&m1);
    bdFree(&m2);
    bdFree(&h);
    bdFree(&hq);
    return 0;
}

#endif

static uint8_t InitKeyEngine()
{
    sj_key_t crypt;
    sj_key_t *p_pain;
    uint8_t key[32] = {"19921210"};

    crypt.data = crypt_rep_table;
    crypt.size = sizeof(crypt_rep_table);
    p_pain = sinjong_aes_decrypt(&crypt, key);
    memcpy(sj_rep_tlb, p_pain->data, p_pain->size);
    free(p_pain->data);
    free(p_pain);

    crypt.data = _36_rep_table;
    crypt.size = sizeof(_36_rep_table);
    p_pain = sinjong_aes_decrypt(&crypt, key);
    memcpy(sj_63_rep_table, p_pain->data, p_pain->size);
    free(p_pain->data);
    free(p_pain);

    return 0;
}
void  _10To36(char *dec, char*threeth)
{

    BIGD bd_Dec = bdNew();
    BIGD bd_R = bdNew();
    BIGD bd_Q = bdNew();
    BIGD bd_Unit = bdNew();
    int i_R;
    char result[256] = { 0 };
    char tmp[256] = { 0 };
    int i = 0;;

    bdConvFromDecimal(bd_Dec, dec);
    bdSetShort(bd_Unit, 35);
    
    do
    {
        bdDivide(bd_Q, bd_R, bd_Dec, bd_Unit);
        bdConvToDecimal(bd_R, tmp, 256);
        bdSetEqual(bd_Dec, bd_Q);
        i_R = atoi(tmp);
        result[i++] = sj_63_rep_table[i_R];
        
    }while (bdShortCmp(bd_Dec, 0) != 0);

    for(i = 0; i < strlen(result); i++)
         threeth[i] = result[strlen(result) - i - 1];   

//    strcpy(threeth, strrev(result));
}

void  _36To10(char*threeth, char *dec)
{

    BIGD bd_Dec = bdNew();
    BIGD bd_tmp = bdNew();
    BIGD bd_Q = bdNew();
    BIGD bd_Unit = bdNew();
    int i_M;
    char result[256] = { 0 };
    char tmp[256] = { 0 };
    int i = 0, j = 0;
    int len = strlen(threeth);

    for (i = 0; i < len; i++)
    {
        i_M = threeth[i] <= '9' ? threeth[i] - '0' : (threeth[i] - 'A' + 10);
        bdSetShort(bd_tmp, 1);
        bdShortMult(bd_tmp, bd_tmp, i_M);
        for (j = (len - i - 1); j > 0; j--)
            bdShortMult(bd_tmp, bd_tmp, 35);
        bdAdd(bd_Dec, bd_Dec, bd_tmp);
    }

    bdConvToDecimal(bd_Dec, dec, 256);
}


void  _36ToOct(char*threeth, char *oct, int*size)
{

    BIGD bd_Dec = bdNew();
    BIGD bd_tmp = bdNew();
    BIGD bd_Q = bdNew();
    BIGD bd_Unit = bdNew();
    int i_M;
    char result[512] = { 0 };
    char tmp[512] = { 0 };
    int i = 0, j = 0;
    int len = strlen(threeth);

    //threeth = strupr(threeth);
    for (i = 0; i < len; i++)
    {
        i_M = (uint8_t*)strchr(sj_63_rep_table, threeth[i]) - sj_63_rep_table;
        bdSetShort(bd_tmp, 1);
        bdShortMult(bd_tmp, bd_tmp, i_M);
        for (j = (len - i - 1); j > 0; j--)
            bdShortMult(bd_tmp, bd_tmp, 33);
        bdAdd(bd_Dec, bd_Dec, bd_tmp);
    }

    *size = bdConvToOctets(bd_Dec, result, sizeof(result));

    memcpy(oct, result + sizeof(result) - *size, *size);
    oct[*size] = 0;
}

static inline void  _OctTo36(char *oct, int size, char*threeth)
{
    BIGD bd_Dec = bdNew();
    BIGD bd_R = bdNew();
    BIGD bd_Q = bdNew();
    BIGD bd_Unit = bdNew();
    int i_R;
    char result[256] = { 0 };
    char tmp[256] = { 0 };
    int i = 0;;

    bdConvFromOctets(bd_Dec, oct, size);
    bdSetShort(bd_Unit, 33);

    do
    {
        bdDivide(bd_Q, bd_R, bd_Dec, bd_Unit);
        bdConvToDecimal(bd_R, tmp, 256);
        bdSetEqual(bd_Dec, bd_Q);
        i_R = atoi(tmp);
        result[i++] = sj_63_rep_table[i_R];
#ifdef _WIN32
        OutputDebugStringA(tmp);
#endif

    } while (bdShortCmp(bd_Dec, 0) != 0);
#ifdef _WIN32
    OutputDebugStringA("\r\n");
#endif

    for(i = 0; i < strlen(result); i++)
         threeth[i] = result[strlen(result) - i - 1];   
    //strcpy(threeth, strrev(result));
}

#if SINJONG_SERVER
uint8_t sinjong_set_private_key(char *pvKey)
{
    sj_key_t key, *sjPVKey;
    int dotPos;

    key.data = pvKey;
    key.size = strlen(pvKey);

    sjPVKey = char2bin(&key);

    memset(sinjong_n, 0, sizeof(sinjong_n));
    memset(sinjong_e, 0, sizeof(sinjong_e));

    dotPos = strchr(sjPVKey->data, ':') - (char*)sjPVKey->data;

    memcpy(sinjong_n, sjPVKey->data, dotPos);
    memcpy(sinjong_e, sjPVKey->data + dotPos + 1, sjPVKey->size - dotPos - 1);

    free(sjPVKey->data);
    free(sjPVKey);

    return 0;
}

uint8_t* sinjong_generate_license(char *machine_id)
{
    char *license;
    sj_key_t plain, *sj_res;
    int i;
    char pad = 0;
    char binaryMachineID[256];
    int binLicesneSize = 0;

    _36ToOct(machine_id, binaryMachineID, &binLicesneSize);

    
    plain.size = binLicesneSize;
    plain.data = binaryMachineID;

    sj_res = sinjong_rsa_encrypt(&plain);

    license = malloc(256);
    memset(license, 0, 256);
  
    _OctTo36(sj_res->data, sj_res->size, license);

    pad = sj_63_rep_table[strlen(sj_63_rep_table) - 1];
    for (i = strlen(license); i < 16; i++)
    {
        license[i] = pad;
    }

    return license;
}
#endif

char* sinjong_decrypt_text(char*_36Text)
{
    char cipher[512] = { 0 };
    int size = 0;

    _36ToOct(_36Text, cipher, &size);

    sj_key_t sj_cipher;
    sj_cipher.data = cipher;
    sj_cipher.size = size;

    sj_key_t *p_tmp = sinjong_aes_decrypt(&sj_cipher, _TextKey);

    char *p = strdup(p_tmp->data);
    free(p_tmp->data);
    free(p_tmp);
    return p;
}

uint8_t sinjong_check_license(char*_license, char*_mid)
{
    int i = 0;
    sj_key_t key;
    char binaryLicense[512] = { 0 };
    sj_key_t *sjRes;
    char szMachine[24] = { 0 };
    int szMachineKey;
    char*license, *mid;
    int lic_size, mid_size;
    char pad = 0;
    lic_size = strlen(_license);
    mid_size = strlen(_mid);

    license = malloc(lic_size + 1);
    mid = malloc(mid_size + 1);
    
    memset(license, 0, lic_size + 1);
    memset(mid, 0, mid_size + 1);
    
    strcpy(license, _license);
    strcpy(mid, _mid);

    _36ToOct(mid, szMachine, &szMachineKey);

    key.data = binaryLicense;

    pad = sj_63_rep_table[strlen(sj_63_rep_table) - 1];

    for (i = 0; i < strlen(license); i++)
    {
        if (license[i] == pad)
            license[i] = 0;
    }

    _36ToOct(license, binaryLicense, &key.size);

    sjRes = sinjong_rsa_decrypt(&key);

    uint8_t checksum = 0;
    for (int i = 0; i < sjRes->size - 2; i++)
    {
        _TextKey[i] = sjRes->data[i] ^ szMachine[i];
        checksum ^= sjRes->data[i];
    }

    if (checksum == sjRes->data[sjRes->size-2] && sjRes->data[sjRes->size - 1] == 0x40)
        return 0;
    else
        return 1;
}

uint8_t sinjong_valid_machineid(char *mid)
{
     char tmp[256];
    uint8_t checksum = 0;
    int i = 0, size;

    _36ToOct(mid, tmp, &size);

    for (i = 0; i < 8; i++)
        checksum ^= tmp[i];

    if (checksum == tmp[8] && tmp[9] == 0x40)
        return 0;
    else
        return 1;
}

uint8_t sinjong_set_public_key(char*puKey)
{
    sj_key_t key, *sjPUKey;
    int dotPos;

    key.size = strlen(puKey);
    key.data = malloc(key.size);
    memcpy(key.data, puKey, key.size);

    sjPUKey = char2bin(&key);

    memset(sinjong_n, 0, sizeof(sinjong_n));
    memset(sinjong_d, 0, sizeof(sinjong_d));

    dotPos = (uint8_t*)strchr(sjPUKey->data, ':') - (uint8_t*)sjPUKey->data;

    memcpy(sinjong_d, sjPUKey->data, dotPos);
    memcpy(sinjong_n, sjPUKey->data + dotPos + 1, sjPUKey->size - dotPos - 1);

    free(sjPUKey->data);
    free(sjPUKey);
    free(key.data);
    return 0;

}


#if _WIN32 && TEST
int main(int argc, char* argv[])
{
    uint8_t src[4256] = "ff";
    uint8_t key[32] = "asdfas";
    sj_key_t src_key, *dst_key1, *dst_key2;
    uint8_t dst[4] = { 0 };
    uint8_t dst1[4] = { 0 };
    //uint8_t temp_E[] = 
    //sinjong_generate_rsa_key(700, "1234567890123478901234");
    uint8_t license_key[] = "2qqvVaTjO56KKu6nVW=KCxLFvuu0RTr=u0Sc59x1sYC0Hny6zmRzeHZ2TLVybOd7T8BdkBNuHK=aQ2ij64sBesyytNkx2Jrw91weOxRrx5tz6X63MH4xcw";
    dst_key1 = sinjong_parser_license_key(license_key, "12345678");
    //dst_key1 = sinjong_generate_license_key("123456", "234", "12345678");
    dst_key1->data[dst_key1->size] = 0;
    printf(dst_key1->data);
    while (1)
    {
        scanf("%s", src);
        
        src_key.data = src;
        src_key.size = strlen(src);

        //dst_key1 = sinjong_rsa_encrypt(&src_key);
        //dst_key1->data[dst_key1->size] = 0;
        printf(dst_key1->data);
        printf("\nsize = %d\n", dst_key1->size);
        dst_key1 = sinjong_rsa_decrypt(dst_key1);
        printf(dst_key1->data);
        printf("\n");
        continue;

        src_key.data = src;
        src_key.size = strlen(src);

        dst_key1 = bin2char(&src_key);
        dst_key2 = char2bin(dst_key1);

        printf("%s\n", src_key.data);
        printf("%s\  src size = %d dst size=%d\n", dst_key2->data, src_key.size, dst_key2->size);
    }
    return 0;
}
#endif