// Byteduino lib - papabyte.com
// MIT License

#ifndef _COSIGNING_H_
#define _COSIGNING_H_

#include "byteduino.hpp"

void handleSignatureRequest(const char senderPubkey[45], nlohmann::json& package);
void treatWaitingSignature();
void stripSignAndAddToConfirmationRoom(const char recipientPubKey[45], nlohmann::json body);
bool acceptToSign(const char * signedTxt);
bool refuseTosign(const char * signedTxt);
void setCbSignatureToConfirm(cbSignatureToConfirm cbToSet);
std::string getOnGoingSignatureJsonString();

#endif