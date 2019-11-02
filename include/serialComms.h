#define SERIAL_RX 16
#define SERIAL_TX 17

HardwareSerial customSerial(2);
bool serial_ready = false;
bool serial_hasMessages = false;
long serial_sendTimer = 0;
NLinkedList<String> serialBufferList = NLinkedList<String>();

void serial_init()
{
    if (global_device.SERIAL_ENABLED)
    {

        customSerial.begin(9600, SERIAL_8N1, SERIAL_RX, SERIAL_TX);
        serial_ready = true;
    }
}

// Add the message to the serial buffer list
void serial_sendString(String msg)
{
    if (global_device.SERIAL_ENABLED)
    {
        serialBufferList.add(msg);
        serial_hasMessages = true;
    }
}

// Send the 0th item to the serial port, then pop the item off
void serial_sendListItem()
{
    customSerial.println(serialBufferList.get(0));
    Serial.print("Serial : ");
    Serial.println(serialBufferList.size());
    Serial.print(" | ");
    Serial.println(serialBufferList.get(0));
    serialBufferList.shift();
}

// Check for updated messages
void serial_processLoop()
{
    if (customSerial.available())
    {
        // Do stuff from the serial port
    }
    if (serial_hasMessages)
    {
        // Send the next message
        if (serialBufferList.size() > 0)
        {
            if ((millis() - serial_sendTimer) > 500)
            {
                serial_sendListItem();
                serial_sendTimer = millis();
            }
        }
        else
        {
            serial_hasMessages = false;
        }
    }
}

void serial_write_str(String msg)
{
    customSerial.println(msg);
    Serial.print("Serial : ");
    Serial.println(msg);
    // customSerial.write(msg);
}