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

#define MEASURE_INTERVAL_MS 30000

#define MQTT_HOST   "aliensdetected.com"
#define MQTT_PORT   1883
#define MQTT_TOPIC  "bertrik/dust"

static SoftwareSerial sensor(PIN_RX, PIN_TX);
static WiFiClient wifiClient;
static WiFiManager wifiManager;
static PubSubClient mqttClient(wifiClient);

static char esp_id[16];
static char device_name[20];

void setup(void)
{
    // welcome message
    Serial.begin(115200);
    Serial.println("SDS011 ESP reader");

    // get ESP id
    sprintf(esp_id, "%06X", ESP.getChipId());
    sprintf(device_name, "SDS011-%s", esp_id);
    Serial.print("Device name: ");
    Serial.println(device_name);

    // connect to wifi or set up captive portal
    Serial.println("Starting WIFI manager ...");
    wifiManager.autoConnect(device_name);

    // initialize the sensor
    sensor.begin(9600);
    SdsInit();

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

static bool mqtt_send_json(const char *topic, const sds_meas_t *sds)
{
    static char json[128];
    char tmp[128];

    // header
    strcpy(json, "{");

    // dust
    sprintf(tmp, "\"SDS011\":{\"id\":\"%04X\",\"PM10\":%.1f,\"PM2.5\":%.1f}", sds->id, sds->pm10, sds->pm2_5);
    strcat(json, tmp);

    // footer
    strcat(json, "}");

    return mqtt_send_string(topic, json);
}


void loop(void)
{
    static sds_meas_t sds_meas;
    static unsigned long last_sent;
    static boolean have_data = false;

    unsigned long ms = millis();

    // check measurement interval
    if ((ms - last_sent) > MEASURE_INTERVAL_MS) {
        if (have_data) {
            // publish it
            char topic[32];
            sprintf(topic, "%s/%s", MQTT_TOPIC, esp_id);
            mqtt_send_json(topic, &sds_meas);
        } else {
            Serial.println("Not publishing, no measurement received from SDS011!");
        }
        last_sent = ms;
        have_data = false;
    }

    // check for incoming measurement data
    while (sensor.available()) {
        uint8_t c = sensor.read();
        if (SdsProcess(c)) {
            // parse it
            SdsParse(&sds_meas);
            have_data = true;
        }
    }
}

