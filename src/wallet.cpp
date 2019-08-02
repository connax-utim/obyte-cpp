// Byteduino lib - papabyte.com
// MIT License

#include "wallet.hpp"

extern walletCreation newWallet;
extern Byteduino byteduino_device;
extern bufferPackageSent bufferForPackageSent;

/* Read nlohmann::json from file
 *
 * Returns nlohmann::json object (via parameter j) */
void readWalletsJson(nlohmann::json& j){  // &
    std::ifstream injson("wallets_created.json", std::ifstream::in);
    injson >> j;
}

/* Read nlohmann::json from file
 *
 * Returns std::string of nlohmann::json object */
std::string getWalletsJsonString(){
	nlohmann::json j;
	readWalletsJson(j);

	std::string returnedString = j.dump();
	return returnedString;
}

/* Save nlohmann::json object of wallets to file
 *
 * */
void saveWalletDefinitionInFlash(const char* wallet, const char* wallet_name, nlohmann::json& wallet_definition_template){

	char output[WALLETS_CREATED_FLASH_SIZE];
    std::string templateString = wallet_definition_template.dump();

    nlohmann::json input;
	readWalletsJson(input);
#ifdef DEBUG_PRINT
	std::clog << std::setw(4) << input << std::endl;
#endif

	if (!input.empty()){
		input["wallet"]["name"] = wallet_name;
		input["wallet"]["definition"] = templateString.c_str();

	#ifdef DEBUG_PRINT
		std::clog << "Save wallet in flash\n" << std::setw(4) << output << std::endl;
	#endif

		std::ofstream outjson("wallets_created.json");
		outjson << std::setw(4) << output << std::endl;
	} else{
#ifdef DEBUG_PRINT
	std::clog << "Impossible to parse input" << std::endl;
#endif
	}
}

/*
 * */
void handleNewWalletRequest(char initiatorPubKey [45], nlohmann::json& package){
	const char* wallet = package["body"]["wallet"].dump().c_str();
	if (wallet != nullptr){
		if (typeid(package["body"]["is_single_address"]) == typeid(bool) && package["body"]["is_single_address"]){
			if(typeid(package["body"]["other_cosigners"]) == typeid(nlohmann::json)){
				int otherCosignersSize = package["body"]["other_cosigners"].size();
				if (!package["body"]["other_cosigners"].empty()){
					const char * initiator_device_hub = package["device_hub"].dump().c_str();
					if (initiator_device_hub != nullptr && strlen(initiator_device_hub) < MAX_HUB_STRING_SIZE){
						newWallet.isCreating = true;
						strcpy(newWallet.initiatorHub, initiator_device_hub);
						memcpy(newWallet.initiatorPubKey,initiatorPubKey,45);
						memcpy(newWallet.id,wallet,45);


						for (int i = 0; i < otherCosignersSize;i++){
							const  char* device_address = package["body"]["other_cosigners"][i]["device_address"].dump().c_str();
							const  char* pubkey = package["body"]["other_cosigners"][i]["pubkey"].dump().c_str();
							const  char* device_hub = package["body"]["other_cosigners"][i]["device_hub"].dump().c_str();

							if (pubkey != nullptr && device_address != nullptr && device_hub != nullptr){
								if (strlen(device_hub) < MAX_HUB_STRING_SIZE){
									if (strcmp(device_address, byteduino_device.deviceAddress) != 0){
										newWallet.xPubKeyQueue[i].isFree = false;
										memcpy(newWallet.xPubKeyQueue[i].recipientPubKey, pubkey,45);
										strcpy(newWallet.xPubKeyQueue[i].recipientHub, device_hub);
									}
								}

							}
						}
					}
					newWallet.xPubKeyQueue[otherCosignersSize+1].isFree = false;
					memcpy(newWallet.xPubKeyQueue[otherCosignersSize+1].recipientPubKey, initiatorPubKey, 45);
					strcpy(newWallet.xPubKeyQueue[otherCosignersSize+1].recipientHub, initiator_device_hub);

					const char* wallet_name = package["body"]["wallet_name"].dump().c_str();
					if (wallet_name != nullptr &&
                            typeid(package["body"]["wallet_definition_template"]) == typeid(nlohmann::json)){
						saveWalletDefinitionInFlash(wallet, wallet_name, package["body"]["wallet_definition_template"]);
					} else {
#ifdef DEBUG_PRINT
						std::clog << "wallet_definition_template and wallet_name must be char" << std::endl;
#endif
					}
				} else {
#ifdef DEBUG_PRINT
					std::clog << "other_cosigners cannot be empty" << std::endl;
#endif
				}
			} else {
#ifdef DEBUG_PRINT
				std::clog << "other_cosigners must be an array" << std::endl;
#endif
			}
		} else {
			std::clog << "Wallet must single address wallet" << std::endl;
		}
	} else {
#ifdef DEBUG_PRINT
		std::clog << "Wallet must be a char" << std::endl;
#endif
	}

}


void treatNewWalletCreation(){

	if (newWallet.isCreating){
		bool isQueueEmpty = true;

		//we send our extended pubkey to every wallet cosigners
		for (int i=0;i<MAX_COSIGNERS;i++){

			if (!newWallet.xPubKeyQueue[i].isFree){
				isQueueEmpty = false;
				if(sendXpubkeyTodevice(newWallet.xPubKeyQueue[i].recipientPubKey, newWallet.xPubKeyQueue[i].recipientHub)){
					newWallet.xPubKeyQueue[i].isFree = true;
				}
			}
		}
		if (isQueueEmpty){
			if (sendWalletFullyApproved(newWallet.initiatorPubKey, newWallet.initiatorHub)){
				newWallet.isCreating = false;
			}
		}
	}
}

bool sendWalletFullyApproved(const char recipientPubKey[45], const char * recipientHub){

	if (bufferForPackageSent.isFree){
		nlohmann::json message = {
			{"from", (const char*) byteduino_device.deviceAddress},
			{"device_hub", (const char*) byteduino_device.hub},
			{"subject", "wallet_fully_approved"},
			{"body",
				{"wallet", (const char*) newWallet.id}
			}
		};

		loadBufferPackageSent(recipientPubKey, recipientHub);
		strcpy(bufferForPackageSent.message, message.dump().c_str());
#ifdef DEBUG_PRINT
		std::clog << bufferForPackageSent.message << std::endl;
#endif
		return true;
	} else {
#ifdef DEBUG_PRINT
		std::clog << "Buffer not free to send message" << std::endl;
#endif
		return false;
	}
}

bool sendXpubkeyTodevice(const char recipientPubKey[45], const char * recipientHub){

	if (bufferForPackageSent.isFree){
		nlohmann::json message = {
			{"from", (const char*) byteduino_device.deviceAddress},
			{"device_hub", (const char*) byteduino_device.hub},
			{"subject", "my_xpubkey"},
			{"body",
	 			{"wallet", (const char*) newWallet.id},
	 			{"my_xpubkey", (const char*) byteduino_device.keys.extPubKey}
	 		}
		};

		loadBufferPackageSent(recipientPubKey, recipientHub);
		strcpy(bufferForPackageSent.message, message.dump().c_str());
#ifdef DEBUG_PRINT
		std::clog << bufferForPackageSent.message << std::endl;
#endif
		return true;
	} else {
#ifdef DEBUG_PRINT
		std::clog << "Buffer not free to send pubkey" << std::endl;
#endif
		return false;
	}
}
