#include <Arduino.h>
#include <TinyGPS++.h>

HardwareSerial SerialGPS(1);
TinyGPSPlus GPS;

void setup()
{
    Serial.begin(115200);
    SerialGPS.begin(9600, SERIAL_8N1, 17, 18);
}

#pragma region GPS
void LerGPS()
{
    while (SerialGPS.available() > 0)
    {
        GPS.encode(SerialGPS.read());
    }
}
void GPSDisponivel()
{
    Serial.print("Satélites: ");
    Serial.println(GPS.satellites.value());
    Serial.print("Latitude: ");
    Serial.println(GPS.location.lat(), 6);
    Serial.print("Longitude: ");
    Serial.println(GPS.location.lng(), 6);
    Serial.print("HDOP: ");
    Serial.println(GPS.hdop.hdop());
    Serial.println("--------------------");
}
void GPSIndisponivel()
{
    Serial.print("Satélites: ");
    Serial.println(GPS.satellites.value());
    Serial.println("Tentando fix...");
    Serial.println("--------------------");
}
#pragma endregion

void loop()
{
    LerGPS();
    if (GPS.satellites.isUpdated())
    {
        if(GPS.location.isValid())
        {
            GPSDisponivel();
        }
        else
        {
            GPSIndisponivel();
        }
    }
}