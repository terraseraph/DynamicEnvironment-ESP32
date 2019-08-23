#include "IPAddress.h"

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include "AsyncUDP.h"

IPAddress getlocalIP();
AsyncUDP udp;

AsyncWebServer server(80);
IPAddress myIP(0, 0, 0, 0);
IPAddress myAPIP(0, 0, 0, 0);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

void handleUpdate(AsyncWebServerRequest *request);
void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);

String body; //for saving body messages
size_t content_len;

//prototype
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);

void webServer_init()
{
    // myAPIP = IPAddress(mesh.getAPIP());

    // WiFi.mode(WIFI_STA);
    // WiFi.begin();

    char *ssid = const_cast<char *>(global_device.AP_SSID.c_str());
    char *pass = const_cast<char *>(global_device.AP_PASSWORD.c_str());

    WiFi.begin(ssid, pass);

    // Serial.println("My AP IP is " + myAPIP.toString());
    Serial.println("My AP IP is " + WiFi.localIP().toString());

    if (udp.listenMulticast(IPAddress(239, 0, 0, 1), 1234))
    {
        Serial.print("UDP Listening on IP: ");
        Serial.println(WiFi.localIP());
        udp.onPacket([](AsyncUDPPacket packet) {
            Serial.print("UDP Packet Type: ");
            Serial.print(packet.isBroadcast() ? "Broadcast" : packet.isMulticast() ? "Multicast" : "Unicast");
            Serial.print(", From: ");
            Serial.print(packet.remoteIP());
            Serial.print(":");
            Serial.print(packet.remotePort());
            Serial.print(", To: ");
            Serial.print(packet.localIP());
            Serial.print(":");
            Serial.print(packet.localPort());
            Serial.print(", Length: ");
            Serial.print(packet.length());
            Serial.print(", Data: ");
            Serial.write(packet.data(), packet.length());
            Serial.println();
            //reply to the client
            packet.printf("Got %u bytes of data", packet.length());
        });
        //Send multicast
        udp.print("Hello!");
        udp.broadcast("==== BROADCASTING ====");
    }
    // String hName = global_device.HOSTNAME;
    // hName += global_device.MY_ID;

    // mesh.setHostname(const_cast<char *>(hName.c_str()));

    //=========================================//
    //==== Web server routes ================//
    //=======================================//

    //TODO: make a form here to update nodes

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    events.onConnect([](AsyncEventSourceClient *client) {
        client->send("hello!", NULL, millis(), 1000);
    });
    server.addHandler(&events);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", http_setupForm());
        if (request->hasArg("BROADCAST"))
        {
            String msg = request->arg("BROADCAST");
            mesh.sendBroadcast(msg);
        }
        if (request->hasArg("command"))
        {
            String msg = request->arg("command");
            processReceivedPacket(0, msg);
        }
        if (request->hasArg("state"))
        {
            String msg = request->arg("state");
            processReceivedPacket(0, msg);
        }
    });

    server.on("/update_settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        global_device.NODE_TYPE = request->arg("node_type");
        global_device.MY_ID = request->arg("my_id");
        global_device.MQTT_TOPIC_SUBSCRIBE = request->arg("mqtt_sub");
        global_device.MQTT_TOPIC_PUBLISH = request->arg("mqtt_pub");
        global_device.AP_SSID = request->arg("ap_ssid");
        global_device.AP_PASSWORD = request->arg("ap_pass");
        String bAdd = request->arg("broker_ip");
        uint8_t ip[4];
        sscanf(bAdd.c_str(), "%u.%u.%u.%u", &ip[0], &ip[1], &ip[2], &ip[3]);
        global_device.BROKER_ADDRESS[0] = ip[0];
        global_device.BROKER_ADDRESS[1] = ip[1];
        global_device.BROKER_ADDRESS[2] = ip[2];
        global_device.BROKER_ADDRESS[3] = ip[3];
        spiffs_updateConfig();
        request->send(200, http_setupForm());
    });

    server.on("/nodes", HTTP_GET, [](AsyncWebServerRequest *request) {
        SimpleList<uint32_t> nodes;
        nodes = mesh.getNodeList();
        String list;
        SimpleList<uint32_t>::iterator node = nodes.begin();
        while (node != nodes.end())
        {
            list = list + (char)*node;
            Serial.printf(" %u", *node);
            node++;
        }
        request->send(200, "text/html", "{\"nodes\":[ " + list + "]}");
    });

    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
        String mem = String(ESP.getFreeHeap());
        request->send(200, "text/html", "{\"memory\":" + mem + "}");
    });

    server.on(
        "/post",
        HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            request->send(200);
            if (!index) //new body
            {
                body = (char *)data;
            }
            else
            {
                body = body + (char *)data;
            }
            if (index + len == total)
            {
                processReceivedPacket(0, body);
                body = "";
            }

            Serial.println("");
            Serial.println("==== post req ====");
            Serial.println("len");
            Serial.println(len);
            Serial.println("index");
            Serial.println(index);
            Serial.println("total");
            Serial.println(total);
            // free(data);
        });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) { handleUpdate(request); });
    server.on("/doUpdate", HTTP_POST,
              [](AsyncWebServerRequest *request) {},
              [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data,
                 size_t len, bool final) { handleDoUpdate(request, filename, index, data, len, final); });

    server.begin();
}

//=========================================//
//==== Web socket routes ================//
//=======================================//
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
        client->printf("Hello Client %u :)", client->id());
        client->ping();
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
    }
    else if (type == WS_EVT_ERROR)
    {
        Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
    }
    else if (type == WS_EVT_PONG)
    {
        Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
    }
    else if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        String msg = "";
        if (info->final && info->index == 0 && info->len == len)
        {
            //the whole message is in a single frame and we got all of it's data
            Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

            if (info->opcode == WS_TEXT)
            {
                for (size_t i = 0; i < info->len; i++)
                {
                    msg += (char)data[i];
                }
            }
            else
            {
                char buff[3];
                for (size_t i = 0; i < info->len; i++)
                {
                    sprintf(buff, "%02x ", (uint8_t)data[i]);
                    msg += buff;
                }
            }
            Serial.printf("%s\n", msg.c_str());

            if (info->opcode == WS_TEXT)
                client->text("I got your text message");
            else
                client->binary("I got your binary message");
        }
        else
        {
            //message is comprised of multiple frames or the frame is split into multiple packets
            if (info->index == 0)
            {
                if (info->num == 0)
                    Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
            }

            Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

            if (info->opcode == WS_TEXT)
            {
                for (size_t i = 0; i < info->len; i++)
                {
                    msg += (char)data[i];
                }
            }
            else
            {
                char buff[3];
                for (size_t i = 0; i < info->len; i++)
                {
                    sprintf(buff, "%02x ", (uint8_t)data[i]);
                    msg += buff;
                }
            }
            Serial.printf("%s\n", msg.c_str());

            if ((info->index + len) == info->len)
            {
                Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
                if (info->final)
                {
                    Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
                    if (info->message_opcode == WS_TEXT)
                        client->text("I got your text message");
                    else
                        client->binary("I got your binary message");
                }
            }
        }
    }
}

// TODO: fill this in
bool http_sendMessage(String message)
{
    return false;
}

IPAddress getlocalIP()
{
    // return IPAddress(mesh.getStationIP());
    return IPAddress(WiFi.localIP());
}

char *IPAddressToString(uint8_t ip)
{
    char result[16];

    sprintf(result, "%d.%d.%d.%d",
            (ip >> 24) & 0xFF,
            (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF,
            (ip)&0xFF);

    return result;
}

String http_setupForm()
{
    String form = ("<form action='/update_settings' method='GET'>\
  Node Type:<br>\
  <input type='text' name='node_type' \
  value='" + global_device.NODE_TYPE +
                   "'> \
  <br>\
  Device ID:<br>\
  <input type='text' name='my_id'  \
  value='" + global_device.MY_ID +
                   "'>  \
  <br><br>\
  Extra MQTT subscribe topic:<br>\
    <input type='text' name='mqtt_sub'  \
  value='" + global_device.MQTT_TOPIC_SUBSCRIBE +
                   "'>  \
  <br><br>\
      Mqtt publish topic:<br>\
    <input type='text' name='mqtt_pub'  \
  value='" + global_device.MQTT_TOPIC_PUBLISH +
                   "'>  \
  <br><br>\
    WiFi SSID:<br>\
    <input type='text' name='ap_ssid'  \
  value='" + global_device.AP_SSID +
                   "'>  \
  <br><br>\
    WiFiPassword:<br>\
    <input type='text' name='ap_pass'  \
  value='" + global_device.AP_PASSWORD +
                   "'>  \
  <br><br>\
    Broker IP Address:<br>\
  <input type='text' name='broker_ip'  \
  value='" + global_device.BROKER_ADDRESS[0] +
                   "." + global_device.BROKER_ADDRESS[1] + "." + global_device.BROKER_ADDRESS[2] + "." + global_device.BROKER_ADDRESS[3] + " \
                   '>  \
  <br><br>\
  <input type='submit' value='Submit'>\
</form>");
    form += "<h3>Hardware ID</h3>";
    form += cmd_getHardwareId();
    form += "<br><br>";
    form += "<h3>Version</h3>";
    form += global_device.VERSION;
    form += "<br><br>";
    form += "<a href='/update'><button>Upload .bin file</button></a>";

    return form;
}

void handleUpdate(AsyncWebServerRequest *request)
{
    char *html = "<form method='POST' action='/doUpdate' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";
    request->send(200, "text/html", html);
}

void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
    if (!index)
    {
        Serial.println("Update");
        content_len = request->contentLength();
        // if filename includes spiffs, update the spiffs partition
        int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
#ifdef ESP8266
        Update.runAsync(true);
        if (!Update.begin(content_len, cmd))
        {
#else
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd))
        {
#endif
            Update.printError(Serial);
        }
    }

    if (Update.write(data, len) != len)
    {
        Update.printError(Serial);
#ifdef ESP8266
    }
    else
    {
        Serial.printf("Progress: %d%%\n", (Update.progress() * 100) / Update.size());
#endif
    }

    if (final)
    {
        AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
        response->addHeader("Refresh", "20");
        response->addHeader("Location", "/");
        request->send(response);
        if (!Update.end(true))
        {
            Update.printError(Serial);
        }
        else
        {
            Serial.println("Update complete");
            Serial.flush();
            ESP.restart();
        }
    }
}