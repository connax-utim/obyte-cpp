// Byteduino lib - papabyte.com
// MIT License
#include "byteduino.hpp"

#ifdef ESP32
hw_timer_t * baseTimer = NULL;
hw_timer_t * watchdogTimer = NULL;
#else
#include <time.h>
clock_t baseTimer;
#endif

// WSClient webSocketForHub;
net::io_context webSocketForHub;
//#if !UNIQUE_WEBSOCKET
//WebSocketsClient secondWebSocket;
//#endif

static volatile bool baseTickOccured = false;
unsigned char job2Seconds = 0;
bufferPackageReceived bufferForPackageReceived;
bufferPackageSent bufferForPackageSent;
Byteduino byteduino_device;

void IRAM_ATTR timerCallback() {
	baseTickOccured = true;
}

void IRAM_ATTR restartDevice() {
//  ESP.restart();
}

void setHub(const char * hub){
	if(strlen(hub) < MAX_HUB_STRING_SIZE){
		strcpy(byteduino_device.hub,hub);
	}
}

void setTestNet(){
	byteduino_device.isTestNet = true;
}

void setDeviceName(const char * deviceName){
	if(strlen(deviceName) < MAX_DEVICE_NAME_STRING_SIZE){
		strcpy(byteduino_device.deviceName, deviceName);
	}
}

void setExtPubKey(const char * extPubKey){
	if(strlen(extPubKey) == 111){
		strcpy(byteduino_device.keys.extPubKey, extPubKey);
	}
}

void setPrivateKeyM1(const char * privKeyB64){
	decodeAndCopyPrivateKey(byteduino_device.keys.privateM1, privKeyB64);
}

void setPrivateKeyM4400(const char * privKeyB64){
	decodeAndCopyPrivateKey(byteduino_device.keys.privateM4400, privKeyB64);
}

bool isDeviceConnectedToHub(){
	return byteduino_device.isConnected;
}


void byteduino_init (){

#if defined(ESP32)
	//watchdog timer
	watchdogTimer = timerBegin(0, 80, true);
	timerWrite(watchdogTimer, 0);
	timerAttachInterrupt(watchdogTimer, &restartDevice, true);  
	timerAlarmWrite(watchdogTimer, 6000 * 1000, false); 
	timerAlarmEnable(watchdogTimer);
#endif


	//calculate device pub key
	getCompressAndEncodePubKey(byteduino_device.keys.privateM1, byteduino_device.keys.publicKeyM1b64);
	
	//calculate wallet pub key
	getCompressAndEncodePubKey(byteduino_device.keys.privateM4400, byteduino_device.keys.publicKeyM4400b64);

	//determine device address
	getDeviceAddress(byteduino_device.keys.publicKeyM1b64, byteduino_device.deviceAddress);
	
	//determine funding address
	getPaymentAddressFromPubKey(byteduino_device.keys.publicKeyM4400b64, byteduino_device.fundingAddress);

	//send device infos to serial
	printDeviceInfos();
	
	//start websocket

    // The io_context is required for all I/O
    WSInitiate(webSocketForHub, getDomain(byteduino_device.hub), byteduino_device.port);


//  webSocketForHub.beginSSL(getDomain(byteduino_device.hub), byteduino_device.port, getPath(byteduino_device.hub));
//	webSocketForHub.onEvent(webSocketEvent);
//	FEED_WATCHDOG;

//#if !UNIQUE_WEBSOCKET
//	secondWebSocket.onEvent(secondWebSocketEvent);
//#endif

	// set up data partition access
	// EEPROM.begin(TOTAL_USED_FLASH);
	const esp_partition_t* obyte = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "obyte_data");

	uECC_set_rng(&getRandomNumbersForUecc);
	loadPreviousMessengerKeys();

	//set up base timer
#if defined(ESP32)
	baseTimer = timerBegin(1, 80, true);
	timerAttachInterrupt(baseTimer, &timerCallback, true);
	timerAlarmWrite(baseTimer, 1000000, true);
	timerAlarmEnable(baseTimer);
#else
    baseTimer = clock();
#endif

	byteduino_device.isInitialized = true;

}

void printDeviceInfos(){
	std::clog << "Device address: " << std::endl;
	std::clog << byteduino_device.deviceAddress << std::endl;
	std::clog << "Device name: " << std::endl;
	std::clog << byteduino_device.deviceName << std::endl;
	std::clog << "Pairing code: " << std::endl;
	std::clog << byteduino_device.keys.publicKeyM1b64 << std::endl;
	std::clog << "@" << std::endl;
	std::clog << byteduino_device.hub << std::endl;
	std::clog << "#0000" << std::endl;
	std::clog << "Extended Pub Key:" << std::endl;
	std::clog << byteduino_device.keys.extPubKey << std::endl;
	std::clog << "Funding address:" << std::endl;
	std::clog << byteduino_device.fundingAddress << std::endl;
}


std::string getDeviceInfosJsonString(){
	nlohmann::json mainObject;

	mainObject["device_address"] = (const char *) byteduino_device.deviceAddress;
	mainObject["device_hub"] = (const char *) byteduino_device.hub;
	mainObject["device_pubkey"] =(const char *) byteduino_device.keys.publicKeyM1b64;
	mainObject["extended_pubkey"] =(const char *) byteduino_device.keys.extPubKey;
	std::string returnedString;
	returnedString = mainObject.dump();
	return returnedString;
}

void webSocketEvent() {
//
//	switch (type) {
//		case WStype_DISCONNECTED:
//			byteduino_device.isConnected = false;
//			std::clog << "Wss disconnected" << std::endl;
//		break;
//		case WStype_CONNECTED:
//		{
//			byteduino_device.isConnected = true;
//			std::clog << "Wss connected to: " << payload << std::endl;
//		}
//		break;
//		case WStype_TEXT:
//			respondToHub(payload);
//	break;
//	}
//
}

void byteduino_loop(){

//	FEED_WATCHDOG;
//#if !UNIQUE_WEBSOCKET
//	secondWebSocket.loop();
//	yield();
//#endif

	updateRandomPool();

	if (!byteduino_device.isInitialized && isRandomGeneratorReady()){
		byteduino_init ();
	}

	//things we do when device is connected
	if (byteduino_device.isConnected){
		treatReceivedPackage();
		treatNewWalletCreation();
		treatWaitingSignature();
		treatSentPackage();
		treatPaymentComposition();

		if (bufferForPackageReceived.hasUnredMessage &&
				bufferForPackageReceived.isFree &&
				!bufferForPackageReceived.isRequestingNewMessage)
			refreshMessagesFromHub();
	}

	//things we do every second
	if (baseTickOccured != 0) {
		job2Seconds++;
		if (job2Seconds == 2){
			if (byteduino_device.isConnected)
				sendHeartbeat();
				job2Seconds = 0;
		}

	baseTickOccured = false;
	managePackageSentTimeOut();
	managePaymentCompositionTimeOut();
	manageMessengerKey();
	}
}
