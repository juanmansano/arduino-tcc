#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <Hash.h>
#include <WebSocketsClient.h>

const char *wlan_ssid_topologia_um = "Mansano";
const char *wlan_password_topologia_um = "m4n54n0x";
const char *wlan_ssid_topologia_dois = "Cavazin";
const char *wlan_password_topologia_dois = "7243883266";
const int topologia = 1;
const char *ws_host = "tccmansano.ddns.net";
const int ws_port = 15674;
const char *stompUrl = "/ws";

ESP8266WebServer server(80);
WebSocketsClient webSocket;

void sendMessage(String &msg)
{
    webSocket.sendTXT(msg.c_str(), msg.length() + 1);
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{

    switch (type)
    {
    case WStype_CONNECTED:
    {
        String msg = "CONNECT\r\naccept-version:1.1,1.0\r\nheart-beat:10000,10000\r\n\r\n";
        sendMessage(msg);
    }
    break;
    case WStype_TEXT:
    {

        String text = (char *)payload;

        if (text.startsWith("MESSAGE"))
        {

            Serial.println(sizeof(payload));

            int index = text.lastIndexOf('\n');
            int length = text.length();
            String sub_S = text.substring(index, length);

            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, sub_S);

            if (error)
            {
                // if the file didn't open, print an error:
                Serial.print(F("Error parsing JSON "));
                Serial.println(error.c_str());
            }
            else
            {
                JsonObject postObj = doc.as<JsonObject>();
                int luminosidade = doc["luminosidade"];
                //                    Serial.println(luminosidade);

                DynamicJsonDocument doc(512);
                doc["status"] = "OK";
                String buf;
                serializeJson(doc, buf);
                server.send(201, F("application/json"), buf);
            }
        }

        else if (text.startsWith("CONNECTED"))
        {

            String msg = "SUBSCRIBE\nid:sub-0\ndestination:/queue/fog\n\n";
            sendMessage(msg);
            delay(1000);
        }

        break;
    }
    }
}

void setActivity()
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
        int json_atividade = doc["atividade"];

        if (server.method() == HTTP_POST)
        {
            if (postObj.containsKey("atividade"))
            {

                String ativ;
                serializeJson(doc, ativ);
                //                Serial.println(ativ);
                String msg = "SEND\ndestination:/queue/end_point\n\n";
                msg.concat(ativ);

                sendMessage(msg);
            }
            else
            {
                DynamicJsonDocument doc(512);
                doc["status"] = "KO";
                doc["message"] = F("No data found, or incorrect!");

                Serial.print(F("Stream..."));
                String buf;
                serializeJson(doc, buf);

                server.send(400, F("application/json"), buf);
                Serial.print(F("done."));
            }
        }
    }
}

// Define routing
void restServerRouting()
{
    server.on("/", HTTP_GET, []()
                { server.send(200, F("text/html"),
                            F("Welcome to the REST Web Server")); });
    // handle post request
    server.on(F("/atividade"), HTTP_POST, setActivity);
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

    WiFi.mode(WIFI_STA);
    if (topologia == 1)
    {
        Serial.print("Logging into WLAN: ");
        Serial.print(wlan_ssid_topologia_um);
        Serial.print(" ...");
        WiFi.begin(wlan_ssid_topologia_um, wlan_password_topologia_um);
    }
    else
    {
        Serial.print("Logging into WLAN: ");
        Serial.print(wlan_ssid_topologia_dois);
        Serial.print(" ...");
        WiFi.begin(wlan_ssid_topologia_dois, wlan_password_topologia_dois);
    }

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println(" success.");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

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

    webSocket.setAuthorization("juan", "juan1234");
    webSocket.begin(ws_host, ws_port, stompUrl);
    webSocket.onEvent(webSocketEvent);
}

void loop()
{
    webSocket.loop();
    server.handleClient();
}
