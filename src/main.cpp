#include <GPS.h>
#include <LED.h>
#include <HTTP.h>
#include <Arduino.h>
#include <WiFiConn.h>

void setup()
{
    Serial.begin(115200);
    InicializarWiFi();
    InicializarLEDs(8, 9);
    InicializarGPS(17, 18);
}

void loop()
{
    VerificarConexaoWiFi();
    LerGPS();
    AtualizarLEDs();
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