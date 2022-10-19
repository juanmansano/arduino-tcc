#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#define WIFI_SSID_TOPOLOGIA_UM "Mansano"
#define WIFI_PASSWORD_TOPOLOGIA_UM "m4n54n0x"
#define WIFI_SSID_TOPOLOGIA_DOIS "Cavazin"
#define WIFI_PASSWORD_TOPOLOGIA_DOIS "7243883266"
#define TOPOLOGIA 1

#define MQTT_HOST "tccmansano.ddns.net"
#define MQTT_PORT 1883
#define MQTT_QOS 0
#define MQTT_PUB "end_point"
#define MQTT_SUB "fog"

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;
ESP8266WebServer server(80);

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

void connectToWifi()
{
  Serial.println("Connecting to Wi-Fi...");
  if (TOPOLOGIA == 1)
  {
    WiFi.begin(WIFI_SSID_TOPOLOGIA_UM, WIFI_PASSWORD_TOPOLOGIA_UM);
  }
  else
  {
    WiFi.begin(WIFI_SSID_TOPOLOGIA_DOIS, WIFI_PASSWORD_TOPOLOGIA_DOIS);
  }
}

void connectToMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  Serial.println("Connected to Wi-Fi.");
  Serial.println(WiFi.localIP());
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool sessionPresent)
{
  Serial.println("Connected to MQTT.");
  uint16_t packetIdSub = mqttClient.subscribe(MQTT_SUB, MQTT_QOS);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected())
  {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
  Serial.println("Subscribe acknowledged.");
}

void onMqttUnsubscribe(uint16_t packetId)
{
  Serial.println("Unsubscribe acknowledged.");
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  String mensagem = payload; 
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, payload);

  if (error)
  {
    // if the file didn't open, print an error:
    Serial.print(F("Error parsing JSON "));
    Serial.println(error.c_str());
  }
  else
  {
    JsonObject postObj = doc.as<JsonObject>();
    
    if (mensagem.indexOf("luminosidade") > 0){
      int luminosidade = doc["luminosidade"];
      Serial.println(luminosidade);
    }

    String buf;

    serializeJson(doc, buf);

    Serial.println(buf);

    server.send(201, F("application/json"), buf);
  }
}

void onMqttPublish(uint16_t packetId)
{
  // Serial.println("Mensagem enviada!");
}

void sendMessage()
{
  String postBody = server.arg("plain");

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postBody);

  if (error)
  {
    // if the file didn't open, print an error:
    Serial.print(F("Error parsing JSON "));
    Serial.println(error.c_str());

    String msg = error.c_str();

    server.send(400, F("text/html"),
                "Error in parsin json body! <br>" + msg);
  }
  else
  {
    JsonObject postObj = doc.as<JsonObject>();

    String msg;
    serializeJson(doc, msg);
    Serial.println(msg.c_str());
    mqttClient.publish(MQTT_PUB, MQTT_QOS, true, msg.c_str());

  }
}

void getAtividades()
{

  char msg[] = "getAtividade";
  mqttClient.publish(MQTT_PUB, MQTT_QOS, true, msg);

}

// Define routing
void restServerRouting()
{
  server.on("/atividades", HTTP_GET, getAtividades);
  // handle post request
  server.on(F("/atividade"), HTTP_POST, sendMessage);
}

// Manage not found URL
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
}

void setup()
{
  Serial.begin(115200);

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setClientId("end_point_");
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();

  if (MDNS.begin("esp8266"))
  {
    Serial.println("MDNS responder started");
  }

  // Set server routing
  restServerRouting();
  // Set not found response
  server.onNotFound(handleNotFound);
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void)
{
  server.handleClient();
}
