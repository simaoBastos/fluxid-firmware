#include <Arduino.h>
#include <WiFiConn.h>

const char* ssid = "brisa-565076";
const char* senha = "blmwfgh5";
bool WiFiJaConectado = false;

void InicializarWiFi()
{
    WiFi.begin(ssid, senha);
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