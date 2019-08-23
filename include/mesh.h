
painlessMesh mesh; // mesh
long mesh_reconnectTime;
long bridgeLastSeen = 0;
long bridgeLastSeenForReset = 0;
bool bridgeConnected = false;

void mesh_init()
{
    mesh.setDebugMsgTypes(ERROR | STARTUP); // set before init() so that you can see startup messages
    // mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);
    // mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL, 0, 10);
    mesh.init(global_device.MESH_SSID, global_device.MESH_PASSKEY, MESH_PORT);
    mesh.onReceive(&mesh_receivedCallback);
    mesh.onNewConnection(&newConnectionCallback);
    mesh.setContainsRoot(true);
}
void meshBridgeInit()
{
    mesh.stationManual(global_device.AP_SSID, global_device.AP_PASSWORD);
    String hName = global_device.HOSTNAME;
    hName += global_device.MY_ID;

    mesh.setHostname(const_cast<char *>(hName.c_str()));
    mesh.setRoot(true);
}

void processMeshLoop()
{
    mesh.update();
    if (global_device.NODE_TYPE != "bridge")
    {
        if ((millis() - bridgeLastSeen) > 1000 * RESTART_FAIL_TIME)
        {
            ESP.restart();
        }
        if ((millis() - bridgeLastSeenForReset) > 1000 * MESH_FAIL_RESTART_TIME)
        {
            mesh_restartMesh();
            bridgeLastSeenForReset = millis();
        }

        if (bridgeConnected)
        {
            bridgeLastSeen = millis();
            bridgeLastSeenForReset = millis();
        }
    }
}

//==============================//
//=====On connection callback===//
//============================//
void newConnectionCallback(uint32_t nodeId)
{
    if (global_device.NODE_TYPE == "bridge")
    {
        String msg = cmd_create_bridgeId(nodeId, false);
        Serial.print("==== Sending bridge setting id to: ");
        Serial.println(nodeId);
        Serial.println(msg);
        mesh_sendSingle(nodeId, msg);
        // mesh.sendSingle(nodeId, msg);
    }
    return;
}

//==============================//
//=====Received callback=======//
//============================//
void mesh_receivedCallback(uint32_t from, String &msg)
{
    parseMeshReceivedPacket(from, msg);
    return;
}

//=================================//
//====== Helpers ==================//
//=================================//

String mesh_getStationIp()
{
    return mesh.getStationIP().toString();
}

bool mesh_sendSingle(uint32_t nodeId, String msg)
{
    return mesh.sendSingle(nodeId, msg);
}

String mesh_getNodeId()
{
    if (global_device.MESH_ENABLED)
    {

        return String(mesh.getNodeId());
    }
    else
    {
        return cmd_getHardwareId();
    }
}

bool mesh_sendBroadcast(String msg)
{
    return mesh.sendBroadcast(msg, false);
}

void mesh_restartMesh()
{
    mesh.stop();
    mesh_init();
}
