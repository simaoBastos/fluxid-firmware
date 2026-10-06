#include <GPS.h>

HardwareSerial SerialGPS(1);
TinyGPSPlus GPS;

DadosGPS dadosGPS;

void InicializarGPS(int8_t pinoRX, int8_t pinoTX)
{
    SerialGPS.begin(9600, SERIAL_8N1, pinoRX, pinoTX);
}

bool DadosGPSAtualizados()
{
   return GPS.satellites.isUpdated();//verifica atualização de satélite pois é a ultima informação que o gps atualiza
}
bool LocalizacaoValida()
{
   return GPS.location.isValid();
}

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
void ImprimirLocalizacaoInvalida()
{
    Serial.print("Satélites: ");
    Serial.println(dadosGPS.satelites);
    Serial.println("Tentando fix...");
    Serial.println("--------------------");
}
