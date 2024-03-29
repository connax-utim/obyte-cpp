//
// Created by tl on 7/25/19.
//

#include "byteduino.hpp"

void setup() {

    setDeviceName("Byteduino");
    setHub("obyte.org/bb");

    //don't forget to change the keys below, you will get troubles if more than 1 device is connected using the same keys
    setPrivateKeyM1("lgVGw/OfKKK4NqtK9fmJjbLCkLv7BGLetrdvsKAngWY=");
    setExtPubKey("xpub6Chxqf8hRQoLRJFS8mhFCDv5yzUNC7rhhh36qqqt1WtAZcmCNhS5pPndqhx8RmgwFhGPa9FYq3iTXNBkYdkrAKJxa7qnahnAvCzKW5dnfJn");
    setPrivateKeyM4400("qA1CxKyzvpKX9+IRhLH8OjLYY06H3RLl+zqc3lT86aI=");

//    Serial.begin(115200);
//    while (!Serial)
//        continue;

    //we don't need an access point
//    WiFi.softAPdisconnect(true);

    //set your wifi credentials here
//    WiFiMulti.addAP("my_wifi_ssid", "my_wifi_password");

//    while (WiFiMulti.run() != WL_CONNECTED) {
//        delay(100);
//        Serial.println(F("Not connected to wifi yet"));
//    }

}

void loop() {
    // put your main code here, to run repeatedly:
    byteduino_loop();
}

int main() {
    setup();
    while(true)
        loop();
}