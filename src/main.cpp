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
struct DadosGPS //struct provisória enquanto o contrato com o WebService não é definido.
                //contém os principais dados do GPS; será ajustada após a definição do contrato.
{
    uint8_t satelites;
    double latitude;
    double longitude;
    double hdop;
};
DadosGPS dadosGPS;
void ObterDadosGPS() 
{
    dadosGPS.satelites = GPS.satellites.value();
    dadosGPS.latitude = GPS.location.lat();
    dadosGPS.longitude = GPS.location.lng();
    dadosGPS.hdop = GPS.hdop.hdop();
}
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
    Serial.println(dadosGPS.satelites);
    Serial.print("Latitude: ");
    Serial.println(dadosGPS.latitude, 6);
    Serial.print("Longitude: ");
    Serial.println(dadosGPS.longitude, 6);
    Serial.print("HDOP: ");
    Serial.println(dadosGPS.hdop);
    Serial.println("--------------------");
}
void GPSIndisponivel()
{
    Serial.print("Satélites: ");
    Serial.println(dadosGPS.satelites);
    Serial.println("Tentando fix...");
    Serial.println("--------------------");
}
#pragma endregion

void loop()
{
    LerGPS();
    if (GPS.satellites.isUpdated())
    {
        ObterDadosGPS();
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