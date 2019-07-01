// Byteduino lib - papabyte.com
// MIT License

#ifndef _WALLET_H_
#define _WALLET_H_

#include "lib/json.hpp"
#include "structures.hpp"
#include "messenger.hpp"
#include <iomanip>
#include <iostream>

void readWalletsJson(nlohmann::json& j);
std::string getWalletsJsonString();
void saveWalletDefinitionInFlash(const char* wallet, const char* wallet_name, nlohmann::json& wallet_definition_template);
void handleNewWalletRequest(char initiatiorPubKey [45], nlohmann::json& package);
void treatNewWalletCreation();
bool sendXpubkeyTodevice(const char recipientPubKey[45], const char * recipientHub);
bool sendWalletFullyApproved(const char recipientPubKey[45], const char * recipientHub);

#endif