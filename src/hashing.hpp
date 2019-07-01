// Byteduino lib - papabyte.com
// MIT License

#ifndef _HASHING_H_
#define _HASHING_H_

#include "byteduino.hpp"
#include "lib/json.hpp"
#include "lib/SHA256.hpp"
#include "lib/ripemd-160.h"

bool getBase64HashForJsonObject (char* hashB64, const nlohmann::json& object);
bool getSHA256ForJsonObject(uint8_t hash[32], const nlohmann::json& object);

void updateHash (classSHA256& hasher, const char * _string, size_t length);
void updateHash (ripemd160_ctx* hasher,const char * _string, size_t length);

void getSHA256(uint8_t * hash, const char * _string, size_t inputLength, size_t outputLength);
void getRipeMD160ForArray(uint8_t hash[20], nlohmann::json object);
void getRipeMD160ForString(uint8_t hash[20] , const char * _string, size_t length);

void getChash160ForString (const char * input, char chash[33]);
void getChash160ForArray (nlohmann::json input, char chash[33]);
void getChash160 (uint8_t * hash160, char chash[33]);

void getDeviceAddress(const char * pubkey, char deviceAddress[34]);
template <class T> bool updateHashForObject (T hasher, const nlohmann::json& object, bool isFirst);
template <class T> void getChash160 (T input, char chash[32]);
template <class T> bool updateHashForArray (T hasher, nlohmann::json array, bool isFirst);
template <class T> bool updateHashForInteger (T hasher, const int number);
template <class T> bool updateHashForChar (T hasher, const char * charToHash);
bool isChar1BeforeChar2(const char* char1, const char* char2);
bool isValidChash160(const char * chash);
unsigned char binArrayToByte (const bool binArray[8]);

#endif