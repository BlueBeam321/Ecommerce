/**
* sj_crypt.h
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
#ifndef _SINJONG_CRYPT_H_
#define _SINJONG_CRYPT_H_
#include <stdint.h>

#define     SINJONG_OK          0
#define     SINJONG_FAIL        -1
#define     SINJONG_TRUE        1
#define     SINJONG_FALSE       0

#define     SINJONG_DEBUG       1
#ifdef _WIN32
#define     SINJONG_SERVER      0
#endif

#define     IN
#define     OUT

#define     SINJONG_DIC_DATA_KEY_SIZE 24
#define     SINJONG_LICENSE_TIME_LIMIT_SIZE 8

typedef enum _sinjong_key_type_e
{
    SINJONG_DIC_DATA_KEY,
    SINJONG_MACHINE_KEY,
    SINJONG_LICENSE_KEY,
    SINJONG_8BDATA_KEY,
    SINJONG_6BDATA_KEY,

}sinjong_key_type_e;
/*
typedef enum _sinjong_error_number_e
{

}sinjong_error_number_e;
*/
typedef struct _sinjong_key_data_t {
    sinjong_key_type_e  type;
    uint8_t             *data;
    int             size;
}sj_key_t;

#if SINJONG_SERVERs
sj_key_t* sinjong_rsa_encrypt(IN sj_key_t *);
uint8_t sinjong_make_rsa_key(int nbits, char *pubkey, char*prvkey);
uint8_t sinjong_set_public_key(char*);
uint8_t sinjong_set_private_key(char*);
sj_key_t* sinjong_aes_encrypt(IN sj_key_t* data, uint8_t* key /*size must 32*/);
void sinjong_generate_rsa_key(int bits, uint8_t*e);
char* sinjong_generate_license(char *);
#endif

enum {
    MID_TYPE_PRODUCT = '0',
    MID_TYPE_SCORE_5000,
    MID_TYPE_SCORE_2000,
    MID_TYPE_SCORE_1000,
    MID_TYPE_SCORE_500,
    MID_TYPE_UNKOWN,
    MID_TYPE_INVALID,
};
//success-0, fail-1
uint8_t sinjong_check_license(char *, char*);
//success-0, fail-1
uint8_t sinjong_valid_machineid(char *);

sj_key_t* sinjong_rsa_decrypt(IN sj_key_t *);
char* sinjong_decrypt_text(char*_36Text);


sj_key_t* sinjong_aes_decrypt(IN sj_key_t* data, uint8_t* key);
sj_key_t* sinjong_parser_license_key(IN uint8_t* message, IN uint8_t* machine_id);
uint8_t* sinjong_generate_machine_id(IN uint8_t*data);


#endif //_SINJONG_CRYPT_H_