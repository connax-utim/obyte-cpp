// Byteduino lib - papabyte.com
// MIT License

#ifndef _PAIRING_H_
#define _PAIRING_H_

#include "lib/json.hpp"
#include "byteduino.hpp"

void readPairedDevicesJson(nlohmann::json& j);
std::string getDevicesJsonString();
void savePeerInFlash(char peerPubkey[45],const char * peerHub, const char * peerName);
void acknowledgePairingRequest(char senderPubkey [45],const char * deviceHub, const char * reversePairingSecret);
void handlePairingRequest(nlohmann::json package);

#endif