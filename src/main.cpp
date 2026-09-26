#include <Arduino.h>

HardwareSerial SerialGPS(1);

void setup()
{
    Serial.begin(115200);
    SerialGPS.begin(9600, SERIAL_8N1, 17, 18);
}

void loop()
{
    while(SerialGPS.available() > 0)
    {
        char c = SerialGPS.read();
        Serial.print(c);
    }

}