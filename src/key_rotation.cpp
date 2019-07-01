// Byteduino lib - papabyte.com
// MIT License
#include "key_rotation.hpp"

messengerKeys myMessengerKeys;

extern net::io_context webSocketForHub;
extern Byteduino byteduino_device;


void getHashToSignForUpdatingMessengerKey(uint8_t* hash) {
	classSHA256 hasher;
	hasher.update("pubkey\0s\0", 9);
	hasher.update( byteduino_device.keys.publicKeyM1b64, strlen(byteduino_device.keys.publicKeyM1b64));
	hasher.update("\0temp_pubkey\0s\0", 15);
	hasher.update(myMessengerKeys.pubKeyB64,strlen(myMessengerKeys.pubKeyB64));
	hasher.finalize(hash,32);
}


void createNewMessengerKeysAndSaveInFlash(){

	uint8_t publicKey[64];
	uint8_t publicKeyCompressed[33];

	nlohmann::json keys = {
		{"private", {}},
		{"public", {}}
	};

	do {
		do{
		}
		while (!getRandomNumbersForPrivateKey(myMessengerKeys.privateKey, 32));
	} while (!uECC_compute_public_key(myMessengerKeys.privateKey, publicKey, uECC_secp256k1()));
		
	uECC_compress(publicKey, publicKeyCompressed, uECC_secp256k1());
	
	Base64.encode(myMessengerKeys.pubKeyB64, (char *) publicKeyCompressed, 33);
	for (int i=0;i<32;i++){
		keys["private"].push_back(myMessengerKeys.privateKey[i]);
	}
	for (int i=0;i<45;i++){
		keys["public"].push_back(myMessengerKeys.pubKeyB64[i]);
	}

	std::ofstream outjson("keys.json");
	outjson << std::setw(4) << keys << std::endl;

}

void loadPreviousMessengerKeys(){
	nlohmann::json keys;
	std::ifstream injson("keys.json");
	injson >> keys;
	int i = 0;
	for (auto it = keys["private"].begin();i<32 && it != keys["private"].end();i++, it++){
		myMessengerKeys.previousPrivateKey[i] = *it;
	}
	i = 0;
	for (auto it = keys["public"].begin() ;i<45 && it != keys["public"].end(); i++, it++){
		myMessengerKeys.previousPubKeyB64[i] = strtol(it->dump().c_str(), nullptr, 10);
	}
}

void manageMessengerKey(){
	if(byteduino_device.isAuthenticated){
		if (byteduino_device.messengerKeyRotationTimer > 0)
			byteduino_device.messengerKeyRotationTimer--;

		if (byteduino_device.messengerKeyRotationTimer == 0 && byteduino_device.isAuthenticated){
			byteduino_device.messengerKeyRotationTimer = SECONDS_BETWEEN_KEY_ROTATION;
			rotateMessengerKey();
		}

	}
}


void rotateMessengerKey() {
	
	createNewMessengerKeysAndSaveInFlash();
	loadPreviousMessengerKeys();

	uint8_t hash[32];
	getHashToSignForUpdatingMessengerKey(hash);

	char sigb64[89];
	getB64SignatureForHash(sigb64, byteduino_device.keys.privateM1, hash, 32);
	char output[300];
	char tag[12];
	getTag(tag,UPDATE_MESSENGER_KEY);
	nlohmann::json mainArray = {
		"request", {
			{"command", "hub/temp_pubkey"},
			{"params", {
				{"temp_pubkey", (const char*) myMessengerKeys.pubKeyB64},
				{"pubkey", (const char*) byteduino_device.keys.publicKeyM1b64},
				{"signature", (const char*) sigb64}
			}},
			{"tag", (const char*) tag}
		}
	};
	strcpy(output, mainArray.dump().c_str());
#ifdef DEBUG_PRINT
	std::clog << output << std::endl;
#endif
	sendTXT(output);
}