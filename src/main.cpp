#include <GPS.h>
#include <HTTP.h>
#include <Arduino.h>
#include <WiFiConn.h>

void setup()
{
    Serial.begin(115200);
    InicializarGPS(17, 18);
    InicializarWiFi();
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
            EnviarDados();
        }
        else
        {
            ImprimirLocalizacaoInvalida();
        }
    }
}