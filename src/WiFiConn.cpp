#include <Arduino.h>
#include <WiFiConn.h>
#include <secrets.h>

bool WiFiJaConectado = false;

void InicializarWiFi()
{
    WiFi.begin(FLUXID_WIFI_SSID, FLUXID_WIFI_PASSWORD);
}

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
