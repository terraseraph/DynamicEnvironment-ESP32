#include "FS.h"
#include <SPIFFS.h>

void spiffs_init();
void spiffs_readConfig();
void spiffs_updateConfig();
void spiffs_writeStringToConfig(String config);
void spiffs_listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void spiffs_readFile(fs::FS &fs, const char *path);
void spiffs_writeFile(fs::FS &fs, const char *path, const char *message);
void spiffs_appendFile(fs::FS &fs, const char *path, const char *message);
void spiffs_renameFile(fs::FS &fs, const char *path1, const char *path2);
void spiffs_deleteFile(fs::FS &fs, const char *path);
void testFileIO(fs::FS &fs, const char *path);

/* Init the SPIFFS read/write for get/set local config */
void spiffs_init()
{
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        Serial.println("Formatting SPIFFS");
        return;
    }
    spiffs_listDir(SPIFFS, "/", 0);
    // spiffs_readFile(SPIFFS, "/config.json");

    File file = SPIFFS.open("/config.json", FILE_READ);
    if (!file || int(file.size() < 20))
    {
        Serial.println("There was an error opening the file for writing");
        file.close();
        spiffs_updateConfig();
        return;
    }
    else
    {
        file.close();
        spiffs_readConfig();
        return;
    }
}

/* 
Updates the current running config to SPIFFS /config.json
 */
void spiffs_updateConfig()
{
    DynamicJsonDocument jsonBuffer(2048);
    jsonBuffer["HARDWARE"] = global_device.HARDWARE;
    jsonBuffer["NODE_TYPE"] = global_device.NODE_TYPE;
    jsonBuffer["MY_ID"] = global_device.MY_ID;
    jsonBuffer["BRIDGE_ID"] = global_device.BRIDGE_ID;
    jsonBuffer["AP_SSID"] = global_device.AP_SSID;
    jsonBuffer["AP_PASSWORD"] = global_device.AP_PASSWORD;
    jsonBuffer["MESH_SSID"] = global_device.MESH_SSID;
    jsonBuffer["MESH_PASSKEY"] = global_device.MESH_PASSKEY;
    jsonBuffer["md5"] = global_device.md5;
    jsonBuffer["noPart"] = global_device.noPart;
    jsonBuffer["partNo"] = global_device.partNo;
    jsonBuffer["MESH_ENABLED"] = global_device.MESH_ENABLED;
    jsonBuffer["MQTT_ENABLED"] = global_device.MQTT_ENABLED;
    jsonBuffer["HTTP_ENABLED"] = global_device.HTTP_ENABLED;
    jsonBuffer["SERIAL_ENABLED"] = global_device.SERIAL_ENABLED;
    jsonBuffer["MQTT_TOPIC_SUBSCRIBE"] = global_device.MQTT_TOPIC_SUBSCRIBE;
    jsonBuffer["MQTT_TOPIC_PUBLISH"] = global_device.MQTT_TOPIC_PUBLISH;
    jsonBuffer["VERSION"] = global_device.VERSION;

    JsonArray brokerAddressArr = jsonBuffer.createNestedArray("BROKER_ADDRESS");
    brokerAddressArr.add(global_device.BROKER_ADDRESS[0]);
    brokerAddressArr.add(global_device.BROKER_ADDRESS[1]);
    brokerAddressArr.add(global_device.BROKER_ADDRESS[2]);
    brokerAddressArr.add(global_device.BROKER_ADDRESS[3]);

    String doc;
    serializeJsonPretty(jsonBuffer, doc);
    spiffs_writeStringToConfig(doc);
    Serial.println(doc);
    ESP.restart();
}

void spiffs_readConfig()
{
    File file = SPIFFS.open("/config.json", FILE_READ);
    DynamicJsonDocument jsonBuffer(2048);
    String rawFile = file.readString();
    Serial.println(int(sizeof(rawFile)));
    deserializeJson(jsonBuffer, rawFile);
    int brokerAddress[4];

    String hardware = jsonBuffer["HARDWARE"];
    String nodeType = jsonBuffer["NODE_TYPE"];
    String myId = jsonBuffer["MY_ID"];
    String bridgeId = jsonBuffer["BRIDGE_ID"];
    String ssid = jsonBuffer["AP_SSID"];
    String password = jsonBuffer["AP_PASSWORD"];
    String meshSsid = jsonBuffer["MESH_SSID"];
    String meshPassword = jsonBuffer["MESH_PASSKEY"];
    String md5 = jsonBuffer["md5"];
    brokerAddress[0] = jsonBuffer["BROKER_ADDRESS"][0];
    brokerAddress[1] = jsonBuffer["BROKER_ADDRESS"][1];
    brokerAddress[2] = jsonBuffer["BROKER_ADDRESS"][2];
    brokerAddress[3] = jsonBuffer["BROKER_ADDRESS"][3];
    String mqttSub = jsonBuffer["MQTT_TOPIC_SUBSCRIBE"];
    String mqttPub = jsonBuffer["MQTT_TOPIC_PUBLISH"];
    String version = jsonBuffer["VERSION"];
    size_t noPart = jsonBuffer["noPart"];
    size_t partNo = jsonBuffer["partNo"];
    bool meshEnabled = jsonBuffer["MESH_ENABLED"];
    bool mqttEnabled = jsonBuffer["MQTT_ENABLED"];
    bool httpEnabled = jsonBuffer["HTTP_ENABLED"];
    bool serialEnabled = jsonBuffer["SERIAL_ENABLED"];

    global_device.HARDWARE = hardware;
    global_device.NODE_TYPE = nodeType;
    global_device.MY_ID = myId;
    global_device.BRIDGE_ID = bridgeId;
    global_device.AP_SSID = ssid;
    global_device.AP_PASSWORD = password;
    global_device.MESH_SSID = meshSsid;
    global_device.MESH_PASSKEY = meshPassword;
    global_device.BROKER_ADDRESS[0] = brokerAddress[0];
    global_device.BROKER_ADDRESS[1] = brokerAddress[1];
    global_device.BROKER_ADDRESS[2] = brokerAddress[2];
    global_device.BROKER_ADDRESS[3] = brokerAddress[3];
    global_device.MQTT_TOPIC_SUBSCRIBE = mqttSub;
    global_device.MQTT_TOPIC_PUBLISH = mqttPub;
    global_device.md5 = md5;
    global_device.noPart = noPart;
    global_device.partNo = partNo;
    global_device.MESH_ENABLED = meshEnabled;
    global_device.MQTT_ENABLED = mqttEnabled;
    global_device.HTTP_ENABLED = httpEnabled;
    global_device.SERIAL_ENABLED = serialEnabled;
    global_device.VERSION = version;

    // String buffer;
    // serializeJsonPretty(jsonBuffer, buffer);
    Serial.println("Stored SPIFFS config.json: ");
    Serial.println(rawFile);
    file.close();
}

void spiffs_writeStringToConfig(String config)
{
    spiffs_writeFile(SPIFFS, "/config.json", const_cast<char *>(config.c_str()));
}

void spiffs_listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if (!root)
    {
        Serial.println("- failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels)
            {
                spiffs_listDir(fs, file.name(), levels - 1);
            }
        }
        else
        {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void spiffs_readFile(fs::FS &fs, const char *path)
{
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if (!file || file.isDirectory())
    {
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while (file.available())
    {
        Serial.write(file.read());
    }
}

void spiffs_writeFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- file written");
    }
    else
    {
        Serial.println("- frite failed");
    }
}

void spiffs_appendFile(fs::FS &fs, const char *path, const char *message)
{
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file)
    {
        Serial.println("- failed to open file for appending");
        return;
    }
    if (file.print(message))
    {
        Serial.println("- message appended");
    }
    else
    {
        Serial.println("- append failed");
    }
}

void spiffs_renameFile(fs::FS &fs, const char *path1, const char *path2)
{
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2))
    {
        Serial.println("- file renamed");
    }
    else
    {
        Serial.println("- rename failed");
    }
}

void spiffs_deleteFile(fs::FS &fs, const char *path)
{
    Serial.printf("Deleting file: %s\r\n", path);
    if (fs.remove(path))
    {
        Serial.println("- file deleted");
    }
    else
    {
        Serial.println("- delete failed");
    }
}

void testFileIO(fs::FS &fs, const char *path)
{
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if (!file)
    {
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing");
    uint32_t start = millis();
    for (i = 0; i < 2048; i++)
    {
        if ((i & 0x001F) == 0x001F)
        {
            Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if (file && !file.isDirectory())
    {
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading");
        while (len)
        {
            size_t toRead = len;
            if (toRead > 512)
            {
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F)
            {
                Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    }
    else
    {
        Serial.println("- failed to open file for reading");
    }
}