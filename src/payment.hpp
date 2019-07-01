// Byteduino lib - papabyte.com
// MIT License

#ifndef _PAYMENT_H_
#define _PAYMENT_H_

#include "lib/json.hpp"
#include "byteduino.hpp"

void requestDefinition(const char* address);
void handleDefinition(const nlohmann::json* receivedObject);
void requestInputsForAmount(int amount, const char * address);
void handleInputsForAmount(nlohmann::json receivedObject, const char * tag);
void getParentsAndLastBallAndWitnesses();
void getPaymentAddressFromPubKey(const char * pubKey, char * paymentAddress);
void handleUnitProps(nlohmann::json receivedObject);
void managePaymentCompositionTimeOut();
int sendPayment(int amount, const char * address, const int payment_id);
void treatPaymentComposition();
void composeAndSendUnit(nlohmann::json arrInputs, int total_amount);
void handlePostJointResponse(nlohmann::json receivedObject, const char * tag);
int loadBufferPayment(const int amount, const bool hasDatafeed, const char * dataFeed, const char * recipientAddress, const int id);
int postDataFeed(const char * key, const char * value, const int id);
void handleBalanceResponse(nlohmann::json receivedObject);
void setCbPaymentResult(cbPaymentResult cbToSet);
void setCbBalancesReceived(cbBalancesReceived cbToSet);
void getAvailableBalances();

#endif