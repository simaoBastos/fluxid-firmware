#include <WiFi.h>
#include <Arduino.h>
#include <TinyGPS++.h>
#include <ArduinoJson.h>

HardwareSerial SerialGPS(1);
TinyGPSPlus GPS;

String MontarJson();


#pragma region GPS
bool DadosGPSAtualizados()
{
   return GPS.satellites.isUpdated();//verifica atualização de satélite pois é a ultima informação que o gps atualiza
}
bool LocalizacaoValida()
{
   return GPS.location.isValid();
}
struct DadosGPS /*struct provisória enquanto o contrato com o WebService não é definido. contém os principais dados do GPS; será ajustada após a definição do contrato.*/
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
void ImprimirLocalizacaoValida()
{
    Serial.println(MontarJson());
}
void ImprimirLocalizacaoInvalida()
{
    Serial.print("Satélites: ");
    Serial.println(dadosGPS.satelites);
    Serial.println("Tentando fix...");
    Serial.println("--------------------");
}
#pragma endregion
#pragma region WiFi
const char* ssid = "brisa-565076";
const char* senha = "blmwfgh5";
bool WiFiJaConectado = false;

void VerificarConexaoWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        if(!WiFiJaConectado)
        {
            WiFiJaConectado = true;
            Serial.println("Wifi Conectado!");
        }
    }
    else
    {
        WiFiJaConectado = false;
    }
} 
#pragma endregion
#pragma region JSON
String MontarJson() // estrutura provisória baseada na struct "DadosGPS"
{
    JsonDocument doc;

    doc["satelites"] = dadosGPS.satelites;
    doc["latitude"] = dadosGPS.latitude;
    doc["longitude"] = dadosGPS.longitude;
    doc["hdop"] = dadosGPS.hdop;

    String dados;
    serializeJson(doc, dados);
    return dados;
}
#pragma endregion

void setup()
{
    Serial.begin(115200);
    WiFi.begin(ssid, senha);
    SerialGPS.begin(9600, SERIAL_8N1, 17, 18);
}

void loop()
{
    VerificarConexaoWiFi();
    LerGPS();
    if (DadosGPSAtualizados())
    {
        ObterDadosGPS();
        if(LocalizacaoValida())
        {
            ImprimirLocalizacaoValida();
        }
        else
        {
            ImprimirLocalizacaoInvalida();
        }
    }
}