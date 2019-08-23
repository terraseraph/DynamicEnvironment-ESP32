//==============================//
//=====Prepare packet=======//
//============================//
//Used for direct input from serial/http/mqtt
void processReceivedPacket(uint32_t from, String &msg)
{
    // Saving logServer
    Serial.println("Perparing packet");
    DynamicJsonDocument jsonBuffer(msg.length() + 1024);

    Serial.println("");
    Serial.print("Length of msg");
    Serial.print(msg.length());
    Serial.print(" / ");
    Serial.println(msg.length() + 1024);

    deserializeJson(jsonBuffer, msg);
    JsonObject root = jsonBuffer.as<JsonObject>();
    String buffer;
    serializeJson(root, buffer);
    if (!root.isNull())
    {

        if (root.containsKey("query"))
        {
            cmd_bridge_query(root);
        }

        // if is a command packet
        if (root.containsKey("command") && root.containsKey("toId"))
        {
            if (root["toId"] == global_device.MY_ID || root["toId"] == mesh_getNodeId())
            {
                cmd_parseCommand(root);
            }
            else //broadcast command to the mesh
            {
                if (global_device.MESH_ENABLED)
                {
                    state_forwardPacketToMesh(buffer, root["toId"]);
                }
            }
        }

        // If is event action packet
        if (root.containsKey("state"))
        {
            if (root["toId"] == global_device.MY_ID || root["toId"] == mesh_getNodeId())
            {
                state_parsePacket(root);
            }
            else
            {
                if (global_device.MESH_ENABLED)
                {
                    state_forwardPacketToMesh(buffer, root["toId"]);
                }
            }
        }
        else //just send it anyway......
        {
            if (global_device.MESH_ENABLED)
            {
                state_forwardPacketToMesh(buffer, root["toId"]);
            }
        }
    }
    else //not a json message
    {
        Serial.println("JSON parsing failed");
        // mesh.sendBroadcast(msg, false);
        // mesh_sendBroadcast(msg);
    }
    Serial.print("Sending message from : ");
    Serial.print(from);
    Serial.print(" message: ");
    Serial.print(msg);
    Serial.flush();
    return;
}

//==============================//
//=====Parse message ==========//
//============================//
void parseMeshReceivedPacket(uint32_t from, String msg)
{
    Serial.println("==== parsing Received packet =======");

    DynamicJsonDocument jsonBuffer(msg.length() + 1024);
    Serial.println("");
    Serial.print("Length of msg");
    Serial.print(msg.length());
    Serial.print(" / ");
    Serial.println(msg.length() + 1024);
    deserializeJson(jsonBuffer, msg);
    JsonObject root = jsonBuffer.as<JsonObject>();
    String serialBuffer;
    serializeJson(root, serialBuffer);
    if (!root.isNull())
    {

        if (root.containsKey("command") && (root["toId"] == global_device.MY_ID || root["toId"] == mesh_getNodeId()))
        {
            cmd_parseCommand(root);
        }

        if (root.containsKey("command") && root["toId"] == "broadcast")
        {
            cmd_broadcast(root);
        }

        if (root.containsKey("heartbeat"))
        {
            addNodeToList(from, root["heartbeat"]["id"], root["heartbeat"]["type"], root["heartbeat"]["memory"]);
        }

        // Event Action
        if (root.containsKey("state"))
        {
            state_parsePacket(root);
            if (global_device.NODE_TYPE == "bridge")
            {
                sendMqttPacket(msg);
                //TODO: maybe serial here too
            }
        }

        else
        {
        }
    }
    return;
}