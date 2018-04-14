#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "Arduino.h"

#include "sds011.h"

#include "SoftwareSerial.h"

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>

// SDS011 pins
#define PIN_RX  D7
#define PIN_TX  D8
#define PIN_RST D3
#define PIN_SET D4

#define MEASURE_INTERVAL_MS 10000

#define MQTT_HOST   "aliensdetected.com"
#define MQTT_PORT   1883
#define MQTT_TOPIC  "bertrik/sds011"

static SoftwareSerial sensor(PIN_RX, PIN_TX);
static WiFiClient wifiClient;
static WiFiManager wifiManager;
static PubSubClient mqttClient(wifiClient);

static char device_name[20];

void setup(void)
{
    uint8_t txbuf[8];
    int txlen;

    // welcome message
    Serial.begin(115200);
    Serial.println("SDS011 ESP reader");

    // get ESP id
    sprintf(device_name, "SDS011-%06X", ESP.getChipId());
    Serial.print("Device name: ");
    Serial.println(device_name);

    // connect to wifi or set up captive portal
    Serial.println("Starting WIFI manager ...");
    wifiManager.autoConnect(device_name);

    // initialize the sensor
    sensor.begin(9600);
    SdsInit();

    pinMode(PIN_RST, INPUT_PULLUP);
    pinMode(PIN_SET, INPUT_PULLUP);

    Serial.println("setup() done");
}

static bool mqtt_send_string(const char *topic, const char *string)
{
    bool result = false;
    if (!mqttClient.connected()) {
        mqttClient.setServer(MQTT_HOST, MQTT_PORT);
        mqttClient.connect(device_name);
    }
    if (mqttClient.connected()) {
        Serial.print("Publishing ");
        Serial.print(string);
        Serial.print(" to ");
        Serial.print(topic);
        Serial.print("...");
        result = mqttClient.publish(topic, string);
        Serial.println(result ? "OK" : "FAIL");
    }
    return result;
}

static bool mqtt_send_value(const char *topic, int value)
{
    char string[16];
    snprintf(string, sizeof(string), "%d", value);
    return mqtt_send_string(topic, string);
}

static bool mqtt_send_json(const char *topic, const sds_meas_t *sds)
{
    static char json[128];
    char tmp[128];

    // header
    strcpy(json, "{");

    // dust
    sprintf(tmp, "\"sds011\":{\"id\":\"%04X\",\"pm10\":%.1f,\"pm2_5\":%.1f}", sds->id, sds->pm10, sds->pm2_5);
    strcat(json, tmp);

    // footer
    strcat(json, "}");

    return mqtt_send_string(topic, json);
}


void loop(void)
{
    static sds_meas_t sds_meas;
    static unsigned long last_sent;

    unsigned long ms = millis();

    // check measurement interval
    if ((ms - last_sent) > MEASURE_INTERVAL_MS) {
        // publish it
        mqtt_send_json(MQTT_TOPIC "/json", &sds_meas);
        last_sent = ms;
    }

    // check for incoming measurement data
    while (sensor.available()) {
        uint8_t c = sensor.read();
        if (SdsProcess(c)) {
            // parse it
            SdsParse(&sds_meas);
        }
    }
}

