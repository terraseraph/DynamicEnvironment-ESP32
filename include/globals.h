//#define ARDUINOJSON_ENABLE_PROGMEM 0
//Global vars
#define MESH_PORT 6666
#define MESH_CHANNEL 11

// #define HOSTNAME "JoatServer"
#define MQTTENABLE
#define BRIDGE // to use when files are combined

#define RESTART_FAIL_TIME 200
#define MESH_FAIL_RESTART_TIME 60
uint32_t BRIDGE_ID;

int GLOBAL_PIN;
String nodeName = "logNode"; // Name needs to be unique
// String MY_ID = "50";
// String NODE_TYPE;

char *MY_ID_CHAR;

int MQTT_BROKER_ADDRESS[4] = {192, 168, 0, 51};
int MQTT_BROKER_PORT = 1883;
const char *MQTT_TOPIC = "root";
// bool MQTT_ENABLED = true;

// bool HTTP_ENABLED = true;

// Scheduler scheduler; //scheduler
// painlessMesh mesh;   // mesh

/* The device config, gets read on startup from SPIFFS
- To update this list, also update spiffs_updateConfig() and the config.json file to reflect */
struct DEVICE
{
    String HOSTNAME = "JOATSERVER_";
    String HARDWARE = "ESP32";
    String NODE_TYPE = "update";
    String MY_ID = "50";
    String BRIDGE_ID = "10";
    String AP_SSID;
    String AP_PASSWORD;
    String MESH_SSID;
    String MESH_PASSKEY;
    String md5;
    int BROKER_ADDRESS[4];
    String MQTT_TOPIC_SUBSCRIBE;
    String MQTT_TOPIC_PUBLISH;
    String VERSION;
    size_t noPart;
    size_t partNo = 0;
    bool MESH_ENABLED = false;
    bool HTTP_ENABLED = true;
    bool SERIAL_ENABLED = true;
    bool MQTT_ENABLED = true;
    String IP_ADDRESS;
};

DEVICE global_device;

//Prototypes

//main.cpp
void startupInitType();
void serialEvent();
void processEventLoop();
void bridge_init();
void commsInit();
void commonLoopProcess();

void createJsonPacket(String fromId, String event, String eventType, String action, String actionType, String data);
void processReceivedPacket(uint32_t from, String &msg);
void addNodeToList(uint32_t nodeId, String myId, String nodeType, String memory);
void taskPrepareMeshReconnect();
void taskPrepareHeartbeat();
void taskBroadcastBridgeId();
void printBridgeStatus();
uint32_t getNodeHardwareId(String id);

char *IPAddressToString(uint8_t ip); //webServer

void processRelayAction(String action, int pinNo);                      //relay
void processMp3Action(String action, uint8_t folderId, uint8_t fileId); //mp3

// MQTT
void mqttCallback(char *topic, byte *payload, unsigned int length); // mqtt
void sendMqttPacket(String packet);                                 //mqtt
String mqtt_getIpAddress();

void newConnectionCallback(uint32_t nodeId);            //mesh
void mesh_receivedCallback(uint32_t from, String &msg); //mesh
bool mesh_sendSingle(uint32_t nodeId, String msg);
String mesh_getStationIp();
String mesh_getNodeId();
void mesh_restartMesh();
bool mesh_sendBroadcast(String msg);

// Serial
void serial_init();
void serial_sendString(String msg);
void serial_write_str(String msg);
void serial_processLoop();

void parseMeshReceivedPacket(uint32_t from, String msg); //main
void printBridgeStatus();

// Commands
String cmd_create_bridgeId(uint32_t nodeId);
void cmd_query_nodeList();
void cmd_query_subConnections();
void cmd_bridgeId(JsonObject cmd);
void cmd_functionChange(JsonObject cmd);
void cmd_admin(JsonObject cmd);
void cmd_setId(JsonObject cmd);
void cmd_setName(JsonObject cmd);
void cmd_branchAddress(JsonObject cmd);
void cmd_mqttMode(JsonObject cmd);
bool validateFunctionChange(String msg);
void cmd_otaUpdate(JsonObject cmd);
String cmd_create_bridgeId(uint32_t nodeId, bool broadcast);
void cmd_customPinInit(JsonObject cmd);
void cmd_customPinToggle(JsonObject cmd);
String cmd_getHardwareId();
void cmd_setWifiSSID(JsonObject cmd);

// SERVO
void servo_init();
void servo_processLoop();
void servo_moveToPositionDegrees(int servoNo, int degrees, int all);
bool servoTimeBufferComplete();
void moveServo(int servoNumber);
void servo_processAction(int servoNo, int moveTo, int all, int servoStepSize);

//OTA
void otaReceiveUpdate(JsonObject root);

// Custom pins
void customPin_init(uint8_t pinNo, bool input);            //command
void customPin_toggle(uint8_t pinNo, bool active);         //command
void processCustomPinAction(uint8_t pinNo, String action); //EventAction

// HTTP
String http_setupForm();

char *getMyIdChar()
{
    global_device.MY_ID.toCharArray(MY_ID_CHAR, global_device.MY_ID.length() + 1);
    return MY_ID_CHAR;
}

//Global strings
const PROGMEM String readyMessage = {"{\"ready\":\"true\"}"};
