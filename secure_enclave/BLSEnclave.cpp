//
// Created by kladko on 8/14/19.
//

#define GMP_WITH_SGX

#include <string.h>
#include <cstdint>
#include "../sgxwallet_common.h"


#include "BLSEnclave.h"
#include "libff/algebra/curves/alt_bn128/alt_bn128_init.hpp"
#include "libff/algebra/curves/alt_bn128/alt_bn128_pp.hpp"

std::string *stringFromKey(libff::alt_bn128_Fr *_key) {

    mpz_t t;
    mpz_init(t);

    _key->as_bigint().to_mpz(t);

    char arr[mpz_sizeinbase(t, 10) + 2];

    char *tmp = mpz_get_str(arr, 10, t);
    mpz_clear(t);

    return new std::string(tmp);
}

std::string *stringFromFq(libff::alt_bn128_Fq *_fq) {

    mpz_t t;
    mpz_init(t);

    _fq->as_bigint().to_mpz(t);

    char arr[mpz_sizeinbase(t, 10) + 2];

    char *tmp = mpz_get_str(arr, 10, t);
    mpz_clear(t);

    return new std::string(tmp);
}

std::string *stringFromG1(libff::alt_bn128_G1 *_g1) {


    _g1->to_affine_coordinates();

    auto sX = stringFromFq(&_g1->X);
    auto sY = stringFromFq(&_g1->Y);

    auto sG1 = new std::string(*sX + ":" + *sY);

    delete (sX);
    delete (sY);

    return sG1;

}


libff::alt_bn128_Fr *keyFromString(const char *_keyString) {

    return new libff::alt_bn128_Fr(_keyString);
}


int inited = 0;

void init() {
    if (inited == 1)
        return;
    inited = 1;
    libff::init_alt_bn128_params();
}

void checkKey(int *err_status, char *err_string, const char *_keyString) {

    uint64_t keyLen = strnlen(_keyString, MAX_KEY_LENGTH);

    // check that key is zero terminated string

    if (keyLen == MAX_KEY_LENGTH) {
        snprintf(err_string, MAX_ERR_LEN, "keyLen != MAX_KEY_LENGTH");
        return;
    }


    *err_status = -2;


    if (_keyString == nullptr) {
        snprintf(err_string, BUF_LEN, "Null key");
        return;
    }

    *err_status = -3;

    // check that key is padded with 0s

    for (int i = keyLen; i < MAX_KEY_LENGTH; i++) {
        if (_keyString[i] != 0) {
            snprintf(err_string, BUF_LEN, "Unpadded key");
        }
    }

    std::string ks(_keyString);

    // std::string  keyString =
    // "4160780231445160889237664391382223604184857153814275770598791864649971919844";

    auto key = keyFromString(ks.c_str());

    auto s1 = stringFromKey(key);

    if (s1->compare(ks) != 0) {
        throw std::exception();
    }

    *err_status = 0;

    return;
}


bool enclave_sign(const char *_keyString, const char *_hashXString, const char *_hashYString,
          char* sig) {


    libff::init_alt_bn128_params();


    auto key = keyFromString(_keyString);

    if (key == nullptr) {
        throw std::exception();
    }

    libff::alt_bn128_Fq hashX(_hashXString);
    libff::alt_bn128_Fq hashY(_hashYString);
    libff::alt_bn128_Fq hashZ = 1;


    libff::alt_bn128_G1 hash(hashX, hashY, hashZ);




    libff::alt_bn128_G1 sign = key->as_bigint() * hash;  // sign

    sign.to_affine_coordinates();



    auto r = stringFromG1(&sign);

    memset(sig, 0, BUF_LEN);



    strncpy(sig, r->c_str(), BUF_LEN);

    delete r;



    return true;


}




