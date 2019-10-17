//
// Created by kladko on 10/3/19.
//

#ifndef SGXD_DKGCRYPTO_H
#define SGXD_DKGCRYPTO_H

#include <string>
#include <vector>

std::string gen_dkg_poly( int _t);

std::vector <std::vector<std::string>> get_verif_vect(const char* encryptedPolyHex, int n, int t);

std::vector<std::string> SplitString(const char* koefs, const char symbol);

std::string get_secret_shares( const char* encryptedPolyHex, const std::string& publicKeys, int n, int t);

#endif //SGXD_DKGCRYPTO_H
