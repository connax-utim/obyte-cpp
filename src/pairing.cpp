// Byteduino lib - papabyte.com
// MIT License
#include "pairing.hpp"

extern Byteduino byteduino_device;
extern bufferPackageReceived bufferForPackageReceived;
extern bufferPackageSent bufferForPackageSent;
extern esp_partition_t* obyte;

void readPairedDevicesJson(nlohmann::json j){
	char firstChar;
	esp_partition_read(obyte, PAIRED_DEVICES, &firstChar, 1);
    char str[PAIRED_DEVICES_FLASH_SIZE];
	if (firstChar == 0x7B){
		int i = -1; 
		do {
			i++;
			esp_partition_read(obyte, PAIRED_DEVICES+i, &str[i], 1);
		}
		while (str[i] != 0x00 && i < (PAIRED_DEVICES_FLASH_SIZE));
		str[PAIRED_DEVICES_FLASH_SIZE-1]=0x00;

	}else{
		str[0] = 0x7B;//{
		str[1] = 0x7D;//}
		str[2] = 0x00;//null
	}
	j = nlohmann::json::parse(str);
}

std::string getDevicesJsonString(){
	char firstChar;
	esp_partition_read(obyte, PAIRED_DEVICES, &firstChar, 1);
	char lastCharRead;
	std::string returnedString;

	if (firstChar == 0x7B){
		int i = -1; 
		do {
			i++;
			esp_partition_read(obyte, PAIRED_DEVICES+i, &lastCharRead, 1);
			if (lastCharRead!= 0x00)
				returnedString += lastCharRead;
		}
		while (lastCharRead != 0x00 && i < (PAIRED_DEVICES_FLASH_SIZE));

	}else{
		returnedString += 0x7B;
		returnedString += 0x7D;
	}
	return returnedString;
}

void savePeerInFlash(char peerPubkey[45],const char * peerHub, const char * peerName){
	char output[PAIRED_DEVICES_FLASH_SIZE];
	nlohmann::json objectPeers;
	readPairedDevicesJson(objectPeers);
#ifdef DEBUG_PRINT
	std::clog << std::setw(4) << objectPeers << std::endl;
#endif
	if (!objectPeers.empty()){
		nlohmann::json objectPeer = {
			{"hub", peerHub},
			{"name", peerName}
		};

		objectPeers[peerPubkey] = objectPeer;

		if (objectPeers.dump().size() < PAIRED_DEVICES_FLASH_SIZE){
			strcpy(output, objectPeers.dump().c_str());
#ifdef DEBUG_PRINT
			std::clog << "Save peer in flash" << std::endl;
			std::clog << std::setw(4) << output << std::endl;
#endif
		} else {
#ifdef DEBUG_PRINT
			std::clog << "No flash available to store peer" << std::endl;
#endif
			return;
		}
		int i = -1; 
		do {
			i++;
			esp_partition_write(obyte, PAIRED_DEVICES+i, output+i, 1);
		}
		while (output[i]!= 0x00 && i < (PAIRED_DEVICES+PAIRED_DEVICES_FLASH_SIZE));
	} else{
#ifdef DEBUG_PRINT
		std::clog << "Impossible to parse objectPeers" << std::endl;
#endif
	}
}


void handlePairingRequest(nlohmann::json package){
	const char * reverse_pairing_secret = package["body"]["reverse_pairing_secret"].dump().c_str();
	const char * device_hub = package["device_hub"].dump().c_str();
	const char * device_name = package["body"]["device_name"].dump().c_str();
	if (reverse_pairing_secret != nullptr){
		if(strlen(reverse_pairing_secret) < MAX_PAIRING_SECRET_STRING_SIZE){
			if (device_hub != nullptr){
				if(strlen(device_hub) < MAX_HUB_STRING_SIZE){
					if (device_name != nullptr){
						if(strlen(device_name) < MAX_DEVICE_NAME_STRING_SIZE){
							acknowledgePairingRequest(bufferForPackageReceived.senderPubkey, device_hub, reverse_pairing_secret);
							savePeerInFlash(bufferForPackageReceived.senderPubkey, device_hub, device_name);
						} else {
#ifdef DEBUG_PRINT
							std::clog << "device_hub is too long" << std::endl;
#endif
						}
					} else {
#ifdef DEBUG_PRINT
						std::clog << "device_name must be a char" << std::endl;
#endif
					}
				} else {
#ifdef DEBUG_PRINT
					std::clog << "device_hub is too long" << std::endl;
#endif
				}
			} else {
#ifdef DEBUG_PRINT
				std::clog << "device_hub must be a char" << std::endl;
#endif
			}
		} else {
#ifdef DEBUG_PRINT
			std::clog << "reverse_pairing_secret is too long" << std::endl;
#endif
		}

	} else {
#ifdef DEBUG_PRINT
		std::clog << "reverse_pairing_secret must be a char" << std::endl;
#endif
	}

}


void acknowledgePairingRequest(char recipientPubKey [45], const char * recipientHub, const char * reversePairingSecret){
	nlohmann::json message, body;
	message["from"] = (const char *) byteduino_device.deviceAddress;
	message["device_hub"] = (const char *) byteduino_device.hub;
	message["subject"] = "pairing";

	body["pairing_secret"] = reversePairingSecret;
	body["device_name"] = (const char *) byteduino_device.deviceName;
	message["body"]= body;

	strcpy(bufferForPackageSent.message, message.dump().c_str());
	loadBufferPackageSent(recipientPubKey, recipientHub);

#ifdef DEBUG_PRINT
	std::clog << std::setw(4) << message << std::endl;
#endif
}