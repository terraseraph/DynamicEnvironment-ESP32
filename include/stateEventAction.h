//prototypes
void state_createAndSendPacket(String fromId, String type, String event, String eventType, String action, String actionType, String data);
void state_parsePacket(JsonObject root);
void state_forwardPacketToMesh(JsonObject root, String toId);
// ===================================================
// =========== Create State Packets to send ==========
// ===================================================

/**
 * Creates an event action packet to jsonBuffer
 * type = event/action
 * returns a String to send over the mesh
 */
void state_createAndSendPacket(String fromId, String type, String event, String eventType, String action, String actionType, String data)
{
  DynamicJsonDocument root(2048);
  // JsonObject &root = jsonBuffer.createObject();
  JsonObject state = root.createNestedObject("state");
  JsonObject state_message = state.createNestedObject("message");

  // ensure action and events are correct for opposing states
  if (type == "event")
  {
    action = "noneA";
    actionType = "noneAT";
  }
  else if (type == "action")
  {
    event = "noneE";
    eventType = "noneET";
  }

  root["toId"] = String(BRIDGE_ID);
  state["type"] = type;
  state_message["fromId"] = fromId;
  state_message["event"] = event;
  state_message["eventType"] = eventType;
  state_message["action"] = action;
  state_message["actionType"] = actionType;
  state_message["wait"] = "0";
  state_message["data"] = data;

  String buffer;
  // root.printTo(buffer);
  serializeJson(root, buffer);
  if (global_device.MESH_ENABLED)
  {
    mesh.sendSingle(BRIDGE_ID, buffer);
  }
  if (global_device.MQTT_ENABLED)
  {
    sendMqttPacket(buffer);
  }
  return;
}

// =============================================
// ================ Parse State Packets ========
// =============================================

/**
 * Parse incoming packet
 * uses new standard
 * 
 */
void state_parsePacket(JsonObject root)
{
  // process data when the action type is known
  const char *state_type = root["state"]["type"]; // "action"
  JsonObject state_message = root["state"]["message"];
  const char *state_message_pkt = root["state"]["message"];
  const char *state_message_toId = state_message["toId"];
  const char *state_message_wait = state_message["wait"];
  const char *state_message_event = state_message["event"];
  const char *state_message_eventType = state_message["eventType"];
  const char *state_message_action = state_message["action"];
  const char *state_message_actionType = state_message["actionType"];
  int intId = root["toId"];
  String strId = root["toId"];

  //Switch data type depending on action to be completed
  // if (root["toId"] == MY_ID || root["toId"] == String(mesh.getNodeId()))
  if (strId == global_device.MY_ID || root["toId"] == String(mesh.getNodeId()) || String(intId) == global_device.MY_ID)
  {
    // Send to pins 16/17 custom serial, this is for special action comms
    if (global_device.SERIAL_ENABLED)
    {
      serial_sendString(root["state"]["message"]);
    }
    // serial_write_str(root["state"]["message"]);

    Serial.println("Packet for me");
    Serial.println(state_message_actionType);
    if (strcmp(state_message_actionType, "customPin") == 0)
    {
      uint8_t pinNo = root["state"]["message"]["data"];
      processCustomPinAction(pinNo, state_message_action);
    }
    if (strcmp(state_message_actionType, "relay") == 0)
    {
      int data = root["state"]["message"]["data"];
      processRelayAction(state_message_action, data);
    }
    if (strcmp(state_message_actionType, "servo") == 0)
    {
      uint8_t invalid = -1;
      int servoNo = root["state"]["message"]["data"][0] | invalid;
      int moveTo = root["state"]["message"]["data"][1] | invalid;
      int moveAll = root["state"]["message"]["data"][2] | invalid;
      int servoStepSize = root["state"]["message"]["data"][3] | invalid;
      if (servoNo == invalid && moveTo == invalid && moveAll == invalid)
      {
        const size_t CAPACITY = JSON_ARRAY_SIZE(3);
        String arrStr = root["state"]["message"]["data"];
        StaticJsonDocument<CAPACITY> doc;
        deserializeJson(doc, arrStr);
        JsonArray arr = doc.as<JsonArray>();
        servoNo = arr[0];
        moveTo = arr[1];
        moveAll = arr[2];
        servoStepSize = arr[3];
      }
      servo_processAction(servoNo, moveTo, moveAll, servoStepSize);
    }
    else if (strcmp(state_message_actionType, "mp3") == 0)
    {
      uint8_t invalid = -1;
      String action = root["state"]["message"]["action"];

      uint8_t folderId = root["state"]["message"]["data"][0] | invalid;
      uint8_t fileId = root["state"]["message"]["data"][1] | invalid;

      if (folderId == invalid && fileId == invalid)
      {
        const size_t CAPACITY = JSON_ARRAY_SIZE(3);
        String arrStr = root["state"]["message"]["data"];
        StaticJsonDocument<CAPACITY> doc;
        deserializeJson(doc, arrStr);
        JsonArray arr = doc.as<JsonArray>();
        folderId = arr[0];
        fileId = arr[1];
      }
      // uint8_t folderId = dat["folder"] | invalid;
      // uint8_t fileId = dat["file"] | invalid;
      processMp3Action(action, folderId, fileId);
    }
    //============= CUSTOM =============

    //==================================
    else //goes to serial
    {
      String msg;
      // root.printTo(msg);
      serializeJson(root, msg);
      Serial.println(msg);
    }
  }
  else
  {
    Serial.printf("Not for me");
    return;
  }
}

void state_forwardPacketToMesh(String buffer, String toId)
{
  uint32_t nodeId = getNodeHardwareId(toId);
  if (nodeId != 0)
  {
    Serial.println("Sending to node" + String(nodeId));
    mesh.sendSingle(nodeId, buffer);
  }
  else
  {
    Serial.println("broadcasting, cannot find node id");
    mesh.sendBroadcast(buffer);
  }
}