//************************************************************
// Switch between being a bridge devie or just a mesh node
// Example commands to send via mesh or serial.
// {"command" : "useMagSwitch", "toId" : "10"} -- tells device 10 to use mag switch
// {"command" : "getMeshNodes", "toId":"1"} -- Gets all mesh nodes and prints fre memory
// {"command" : "setId", "toId" : "10", "nodeId" : "11"} (toId also hardware id) -- sets the easy id
// {"toId": "11", "wait": "0", "event":"noneE", "eventType":"noneET", "action":"start", "actionType":"relay", "data":"1"} -- example action
// {"toId":"3213429781","fromId":"10","event":"code","eventType":"keypad","action":"noneA", "actionType" : "noneAT", "data" : "333333"} -- example event
//
// https://randomnerdtutorials.com/esp32-pinout-reference-gpios/ <- for the pins
//************************************************************
// #define ESP32
#define ARDUINOJSON_ENABLE_STD_STRING 1
#include <Arduino.h>
#include <painlessMesh.h>
#include <HardwareSerial.h>
#include <Bounce2.h>
#include <Servo.h>
#include "LinkedList.h"
#include "ota.h"
#include "globals.h"
#include "serialComms.h"
#include "joatSpiffs.h"
#include "mesh.h"
#include "joatEEPROM.h"
#include "joatWiFi.h"
#include "commands.h"
#include "espServo.h"
#include "mp3Player.h"
#include "stateEventAction.h"
#include "webServer.h"
#include "mqtt.h"
#include "joatKeypad.h"
#include "messageProcessing.h"
#include "relay.h"
#include "button.h"
#include "magSwitch.h"
#include "rfid.h"
//------ Custom ------//
#include "custom/phone.h"
#include "custom/customPinMode.h"

String inputString = ""; //for serial input

//==============================//
//===== Scheduler Init ========//
//============================//

// Scheduler reconnectScheduler;
Task taskReconnect(TASK_SECOND * 180, TASK_FOREVER, &taskPrepareMeshReconnect);

Scheduler heartbeatScheduler;
Task taskHeartbeat(TASK_SECOND * 30, TASK_FOREVER, &taskPrepareHeartbeat);

// Scheduler nodeListScheduler;
Task printBridgeStatusTask(TASK_SECOND * 60, TASK_FOREVER, &printBridgeStatus);

Scheduler bridgeIdScheduler;
Task taskBridgeId(TASK_SECOND * 20, TASK_FOREVER, &taskBroadcastBridgeId);

//==============================//
//===== Setup =================//
//============================//

 #include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

void setup()
{
  delay(100);
  // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  btStop();
  Serial.begin(115200);

  // Load config from spiffs
  spiffs_init();
  commsInit();

  Serial.println("======= NodeId " + global_device.MY_ID + "=========");

  startupInitType();

  if (global_device.NODE_TYPE != "bridge")
  {
    heartbeatScheduler.addTask(taskHeartbeat);
    taskHeartbeat.enable();
  }
}

void loop()
{
  serialEvent();
  commonLoopProcess();
  processEventLoop();
}

//==============================//
//=====Startup Init ===========//
//============================//

void commsInit()
{
  if (global_device.MESH_ENABLED)
  {
    Serial.println("Mesh Enabled");
    mesh_init();
  }
  if (global_device.MQTT_ENABLED)
  {
    Serial.println("MQTT enabled");
    mqtt_init();
  }
  if (global_device.HTTP_ENABLED)
  {
    Serial.println("HTTP enabled");
    webServer_init();
  }
}

void startupInitType()
{
  // global_device.NODE_TYPE = getNodeType();
  if (global_device.NODE_TYPE == "relay")
  {
    relay_init();
  };
  if (global_device.NODE_TYPE == "button")
  {
    button_init();
  };
  if (global_device.NODE_TYPE == "keypad")
  {
    keypad_init();
  };
  if (global_device.NODE_TYPE == "magSwitch")
  {
    magSwitch_init();
  };
  if (global_device.NODE_TYPE == "rfid")
  {
    rfid_init();
  };
  if (global_device.NODE_TYPE == "mp3")
  {
    MP3_init();
  };
  if (global_device.NODE_TYPE == "servo")
  {
    servo_init();
  };
  //======= Custom ====================//
  if (global_device.NODE_TYPE == "phone")
  {
    phone_init();
  };
  //==================================//
  if (global_device.NODE_TYPE == "bridge")
  {
    bridge_init();
    bridgeIdScheduler.addTask(printBridgeStatusTask);
    bridgeIdScheduler.addTask(taskBridgeId);
    printBridgeStatusTask.enable();
    taskBridgeId.enable();
  };

  Serial.println("======  Using " + global_device.NODE_TYPE + " =====");
}

//==============================//
//=====Event check loop =======//
//============================//

void commonLoopProcess()
{
  if (global_device.MESH_ENABLED)
  {
    processMeshLoop();
  }
  if (global_device.MQTT_ENABLED)
  {
    processMqtt();
  }
  if (global_device.SERIAL_ENABLED)
  {
    serial_processLoop();
  }
}

void processEventLoop()
{
  if (global_device.NODE_TYPE == "button")
  {
    processButtonEvent();
  };
  if (global_device.NODE_TYPE == "keypad")
  {
    ProcessKeyPad();
  };
  if (global_device.NODE_TYPE == "magSwitch")
  {
    processMagSwitchEvent();
  };
  if (global_device.NODE_TYPE == "relay")
  {
    relayScheduler.execute(); //for relay delays
  };
  if (global_device.NODE_TYPE == "rfid")
  {
    rdidScheduler.execute(); //for relay delays
    processRfid();
  };
  if (global_device.NODE_TYPE == "mp3")
  {
    processMp3Loop();
  };
  if (global_device.NODE_TYPE == "servo")
  {
    servo_processLoop();
  };
  //======= Custom ====================//
  if (global_device.NODE_TYPE == "phone")
  {
    processPhoneLoop();
  };
  //==================================//
  if (global_device.NODE_TYPE != "bridge") //run heartbeat if not the bridge
  {
    heartbeatScheduler.execute();
    // serial_processLoop();
  }
  if (global_device.NODE_TYPE == "bridge") //if it is the bridge
  {
    bridgeIdScheduler.execute();
  }
}

//==============================//
//=====Bridge Init ============//
//============================//
void bridge_init()
{
  meshBridgeInit();
  if (global_device.MQTT_ENABLED)
  {
    mqtt_init();
  }
  if (global_device.HTTP_ENABLED)
  {
    webServer_init();
  }
}

//==========================================//
//===== List of connected nodes ===========//
//========================================//

class MeshNode
{
public:
  uint32_t hardwareId;
  String id;
  String nodeType;
  String memory;
};
NLinkedList<MeshNode *> meshNodeList = NLinkedList<MeshNode *>();

uint32_t getNodeHardwareId(String id)
{
  MeshNode *node;
  for (int i = 0; i < meshNodeList.size(); i++)
  {
    node = meshNodeList.get(i);
    if (node->id == id)
    {
      return node->hardwareId;
    }
  }
  return 0;
}

void addNodeToList(uint32_t nodeId, String myId, String nodeType, String memory)
{
  MeshNode *node;
  bool nodeExists = false;
  for (int i = 0; i < meshNodeList.size(); i++)
  {
    node = meshNodeList.get(i);
    if (node->hardwareId == nodeId)
    {
      nodeExists = true;

      node->id = myId;
      node->nodeType = nodeType;
      node->memory = memory;
    }
  }
  if (!nodeExists)
  {
    node = new MeshNode();
    node->hardwareId = nodeId;
    node->id = myId;
    node->nodeType = nodeType;
    node->memory = memory;
    meshNodeList.add(node);
  }

  String nodeMeshId = String(nodeId); //convert node name to string
  String msg = "{\"heartbeat\":{\"hardwareId\":" + nodeMeshId + ",\"id\":\"" + myId + "\",\"type\":\"" + nodeType + "\",\"memory\":\"" + memory + "\"}}";
  Serial.println(msg);
  if (global_device.MQTT_ENABLED)
  {
    sendMqttPacket(msg);
  }
}

/**
 * Print node list
 */
void printBridgeStatus()
{
  // String msgIp = mesh.getStationIP().toString();
  String msgIp = mesh_getStationIp();
  String mem = String(ESP.getFreeHeap());

  Serial.println("============= Nodes ==========");
  String msg = "{\"bridgeMemory\":" + mem + ",\"ipAddress\":\"" + msgIp + "\"}";
  Serial.println(msg);
  if (global_device.MQTT_ENABLED && global_device.NODE_TYPE == "bridge")
  {
    sendMqttPacket(msg);
  }
  return;
}

void removeNodeFromList(String nodeName)
{
  // nodeList.remove(nodeName);
}

//==============================//
//=====On connection callback===//
//============================//
// void newConnectionCallback(uint32_t nodeId)
// {
//   if (global_device.NODE_TYPE == "bridge")
//   {
//     String msg = cmd_create_bridgeId(nodeId, false);
//     Serial.print("==== Sending bridge setting id to: ");
//     Serial.println(nodeId);
//     Serial.println(msg);
//     mesh_sendSingle(nodeId, msg);
//     // mesh.sendSingle(nodeId, msg);
//   }
//   return;
// }

//==============================//
//=====Parse serial=======//
//============================//
void serialEvent()
{
  while (Serial.available())
  {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n')
    {
      processReceivedPacket(mesh.getNodeId(), inputString);
      inputString = "";
      Serial.println(readyMessage); //tells the server we are ready for another message
    }
  }
  return;
}

// //==============================//
// //=====Prepare packet=======//
// //============================//
// //Used for direct input from serial/http/mqtt
// void preparePacketForMesh(uint32_t from, String &msg)
// {
//   // Saving logServer
//   Serial.println("Perparing packet");
//   DynamicJsonDocument jsonBuffer(msg.length() + 1024);

//   Serial.println("");
//   Serial.print("Length of msg");
//   Serial.print(msg.length());
//   Serial.print(" / ");
//   Serial.println(msg.length() + 1024);

//   deserializeJson(jsonBuffer, msg);
//   JsonObject root = jsonBuffer.as<JsonObject>();
//   String buffer;
//   serializeJson(root, buffer);
//   if (!root.isNull())
//   {

//     if (root.containsKey("query"))
//     {
//       cmd_bridge_query(root);
//     }
//     // if is a command packet
//     if (root.containsKey("command") && root.containsKey("toId"))
//     {
//       if (root["toId"] == global_device.MY_ID || root["toId"] == mesh_getNodeId())
//       {
//         cmd_parseCommand(root);
//       }
//       else //broadcast command to the mesh
//       {
//         state_forwardPacketToMesh(buffer, root["toId"]);
//         // mesh.sendBroadcast(buffer);
//       }
//     }

//     // If is event action packet
//     if (root.containsKey("state"))
//     {
//       if (root["toId"] == global_device.MY_ID || root["toId"] == mesh_getNodeId())
//       {
//         state_parsePacket(root);
//       }
//       else
//       {
//         state_forwardPacketToMesh(buffer, root["toId"]);
//       }
//     }
//     else //just send it anyway......
//     {
//       state_forwardPacketToMesh(buffer, root["toId"]);
//       // mesh.sendBroadcast(buffer);
//     }
//   }
//   else //not a json message
//   {
//     Serial.println("JSON parsing failed");
//     // mesh.sendBroadcast(msg, false);
//     // mesh_sendBroadcast(msg);
//   }
//   Serial.print("Sending message from : ");
//   Serial.print(from);
//   Serial.print(" message: ");
//   Serial.print(msg);
//   Serial.flush();
//   return;
// }

//====================================//
//===== Scheduled Task functions ====//
//==================================//
void taskPrepareHeartbeat()
{
  DynamicJsonDocument root(1024);
  // JsonObject &root = jsonBuffer.createObject();
  JsonObject heartbeat = root.createNestedObject("heartbeat");

  // String hwId = String(mesh.getNodeId());

  String hwId = cmd_getHardwareId();
  String mem = String(ESP.getFreeHeap());
  String msgBuffer;
  heartbeat["hardwareId"] = hwId;
  heartbeat["id"] = global_device.MY_ID;
  heartbeat["type"] = global_device.NODE_TYPE;
  heartbeat["ipAddress"] = global_device.IP_ADDRESS;
  heartbeat["memory"] = mem;

  // root.printTo(msgBuffer);
  serializeJson(root, msgBuffer);
  Serial.println("====Sending heartbeat to: MQTT Broker =====");
  Serial.println(msgBuffer);
  // bridgeConnected = mesh.sendSingle(BRIDGE_ID, msgBuffer);
  bridgeConnected = mesh_sendSingle(BRIDGE_ID, msgBuffer);
  sendMqttPacket(msgBuffer);
}

void taskBroadcastBridgeId()
{
  String msg = cmd_create_bridgeId(0, true);
  Serial.println("==== Broadcasting bridge id ===== ");
  // mesh.sendBroadcast(msg);
  mesh_sendBroadcast(msg);
}

void taskPrepareMeshReconnect()
{
  if (BRIDGE_ID == 0)
  {
    Serial.println("=== Bridge unset, restarting mesh connection ===");
    // mesh.stop();
    // mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, 6);
    mesh_restartMesh();
  }
}

// //==============================//
// //=====Received callback=======//
// //============================//
// void mesh_receivedCallback(uint32_t from, String &msg)
// {
//   parseReceivedPacket(from, msg);
//   return;
// }

// //==============================//
// //=====Parse message ==========//
// //============================//
// void parseReceivedPacket(uint32_t from, String msg)
// {
//   Serial.println("==== parsing Received packet =======");

//   DynamicJsonDocument jsonBuffer(msg.length() + 1024);
//   Serial.println("");
//   Serial.print("Length of msg");
//   Serial.print(msg.length());
//   Serial.print(" / ");
//   Serial.println(msg.length() + 1024);
//   deserializeJson(jsonBuffer, msg);
//   JsonObject root = jsonBuffer.as<JsonObject>();
//   String serialBuffer;
//   serializeJson(root, serialBuffer);
//   if (!root.isNull())
//   {

//     if (root.containsKey("command") && (root["toId"] == global_device.MY_ID || root["toId"] == mesh_getNodeId()))
//     {
//       cmd_parseCommand(root);
//     }

//     if (root.containsKey("command") && root["toId"] == "broadcast")
//     {
//       cmd_broadcast(root);
//     }

//     if (root.containsKey("heartbeat"))
//     {
//       addNodeToList(from, root["heartbeat"]["id"], root["heartbeat"]["type"], root["heartbeat"]["memory"]);
//     }

//     // Event Action
//     if (root.containsKey("state"))
//     {
//       state_parsePacket(root);
//       if (global_device.NODE_TYPE == "bridge")
//       {
//         sendMqttPacket(msg);
//         //TODO: maybe serial here too
//       }
//     }

//     else
//     {
//     }
//   }
//   return;
// }
