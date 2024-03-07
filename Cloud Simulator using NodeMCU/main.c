#include "UbidotsMicroESP8266.h"
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#define TOKEN  "BBFF-kx6Kcq4YBw0MLxKucx0ne7k7rND912" 
#define PIN D1
#define NUMPIXELS 30
#define ID  "5c69a21f93f3c3044223677b"  
#define WIFISSID "WIFI@MMU" 
#define PASSWORD "wifi@mmu" 
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
LiquidCrystal_I2C lcd(0x3F, 16, 2);//LCD
char ssid[] = "WIFI@MMU";
char pass[] = "wifi@mmu";
WiFiClient client;

const char server[] = "api.openweathermap.org";
String nameOfCity = "Cyberjaya,MY";
String apiKey = "5e1247cdf25f0bd487c45e8538f8e9e6";
String text;
int jsonend = 0;
boolean startJson = false;
int status = WL_IDLE_STATUS;
#define JSON_BUFF_DIMENSION 2500
unsigned long lastConnectionTime = 10 * 60 * 1000;
const unsigned long postInterval = 10 * 60 * 1000;
Ubidots client1(TOKEN);

void setup() {
    lcd.begin(16, 2);
    lcd.init();
    lcd.print("hello, world!");
    pinMode(PIN, OUTPUT);
    pixels.begin();
    Serial.begin(115200);
    client1.wifiConnection(WIFISSID, PASSWORD);
    text.reserve(JSON_BUFF_DIMENSION);
    WiFi.begin(ssid, pass);
    Serial.println("connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi Connected");
    printWiFiStatus();
}

void loop() {
    lcd.setCursor(0, 1);
    lcd.print(millis() / 1000);
    int i = 0;
    if (millis() - lastConnectionTime > postInterval) {
        lastConnectionTime = millis();
        makehttpRequest();
    }
    while (i < 10000) {
        if (i == 9999) {
            Serial.println("Resetting ESP");
            ESP.restart(); //ESP.reset();
        }
        i++;
    }
}

// print Wifi status
void printWiFiStatus() {
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

// to request data from OWM
void makehttpRequest() {
    client.stop();
    if (client.connect(server, 80)) {
        client.println("GET /data/2.5/forecast?q=" + nameOfCity + "&APPID=" + apiKey + "&mode=json&units=metric&cnt=2 HTTP/1.1");
        client.println("Host: api.openweathermap.org");
        client.println("User-Agent: ArduinoWiFi/1.1");
        client.println("Connection: close");
        client.println();
        unsigned long timeout = millis();
        while (client.available() == 0) {
            if (millis() - timeout > 5000) {
                Serial.println(">>> Client Timeout !");
                client.stop();
                return;
            }
        }

        char c = 0;
        while (client.available()) {
            c = client.read();
            if (c == '{') {
                startJson = true;
                jsonend++;
            }
            if (c == '}') {
                jsonend--;
            }
            if (startJson == true) {
                text += c;
            }
            if (jsonend == 0 && startJson == true) {
                parseJson(text.c_str());
                text = "";
                startJson = false;
            }
        }
    }
    else {
        Serial.println("connection failed");
        return;
    }

}


//to parse json data recieved from OWM
void parseJson(const char* jsonString) {
    //StaticJsonBuffer<4000> jsonBuffer;
    const size_t bufferSize = 2 * JSON_ARRAY_SIZE(1) + JSON_ARRAY_SIZE(2) + 4 * JSON_OBJECT_SIZE(1) + 3 * JSON_OBJECT_SIZE(2) + 3 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 2 * JSON_OBJECT_SIZE(7) + 2 * JSON_OBJECT_SIZE(8) + 720;
    DynamicJsonBuffer jsonBuffer(bufferSize);

    // FIND FIELDS IN JSON TREE
    JsonObject& root = jsonBuffer.parseObject(jsonString);
    if (!root.success()) {
        Serial.println("parseObject() failed");
        return;
    }

    JsonArray& list = root["list"];
    JsonObject& nowT = list[0];
    JsonObject& later = list[1];

    String city = root["city"]["name"];
    float tempNow = nowT["main"]["temp"];
    float humidityNow = nowT["main"]["humidity"];
    String weatherNow = nowT["weather"][0]["description"];
    float tempLater = later["main"]["temp"];
    float humidityLater = later["main"]["humidity"];
    String weatherLater = later["weather"][0]["description"];
    client1.add("temperature", tempNow);
    client1.sendAll(true);
    Serial.print("Value Sent: ");
    Serial.println(tempNow);
    delay(5000);
    float value1 = client1.getValue(ID);
    Serial.print("Value Retrieved: ");
    Serial.println(value1);
    delay(5000);
    diffDataAction(weatherNow, weatherLater, weatherNow);
    Serial.println();
}

//representing the data
void diffDataAction(String nowT, String later, String weatherType) {
    int j = 0;
    if (weatherType == "rain") {
        for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, pixels.Color(0, 0, 139));
            pixels.show();
        }
        Serial.println("Oh no! It is going to " + weatherType + " later! Predicted " + later);
    }
    else if (weatherType == "light rain") {
        for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, pixels.Color(0, 191, 255));
            pixels.show();
        }
        Serial.println("Oh no! It is going to " + weatherType + " later! Predicted " + later);
    }
    else if (weatherType == "shower rain") {
        for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, pixels.Color(135, 206, 250));
            pixels.show();
        }
        Serial.println("Oh no! It is going to " + weatherType + " later! Predicted " + later);
    }
    else if (weatherType == "clear sky") {
        for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, pixels.Color(255, 140, 0));
            pixels.show();
        }
        Serial.println("Oh no! It is going to " + weatherType + " later! Predicted " + later);
    }
    else if (weatherType == "thunderstorm") {
        while (j < 100) {
            for (int i = 0; i < NUMPIXELS; i++) {
                pixels.setPixelColor(i, pixels.Color(0, 255, 255));
                pixels.show();
                delay(1000);
                pixels.setPixelColor(i, pixels.Color(255, 0, 0));
                pixels.show();
                delay(1000);
            }
            j++;
        }
        Serial.println("Oh no! It is going to " + weatherType + " later! Predicted " + later);
    }
    else if (weatherType == "broken clouds") {
        for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, pixels.Color(255, 255, 255));
            pixels.show();
        }
        Serial.println("Oh no! It is going to " + weatherType + " later! Predicted " + later);
    }
    else {
        Serial.println("It is going to be sunny later! Predicted " + later);
        for (int i = 0; i < NUMPIXELS; i++) {
            pixels.setPixelColor(i, pixels.Color(128, 0, 128));
            pixels.show();
        }
    }
    Serial.print("Finish");
}
