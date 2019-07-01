// Byteduino lib - papabyte.com
// MIT License

#ifndef _SIGNATURE_H_
#define _SIGNATURE_H_

#include "byteduino.hpp"
//#include <inttypes.h>

void decodeAndDecompressPubKey(const char * pubkey, uint8_t decompressedPubkey[64]);
void getB64SignatureForHash(char * sigb64, const uint8_t * privateKey,const uint8_t * hash,size_t length);
void getCompressAndEncodePubKey (const uint8_t * privateKey, char * pubkeyB64);
bool decodeAndCopyPrivateKey(uint8_t * decodedPrivKey, const char * privKeyB64);

#endif