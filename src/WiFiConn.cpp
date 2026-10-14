#include <Arduino.h>
#include <WiFiConn.h>
#include <secrets.h>

bool WiFiJaConectado = false;

namespace
{
    uint32_t ultimaTentativaWiFi = 0;
    const uint32_t INTERVALO_RECONEXAO = 10000UL;
}

void WiFiEvento(
    WiFiEvent_t evento,
    WiFiEventInfo_t informacao
)
{
    if (evento == ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
    {
        uint8_t motivo =
            informacao.wifi_sta_disconnected.reason;

        Serial.print("Wi-Fi desconectado. Codigo: ");
        Serial.println(motivo);

        Serial.print("Motivo: ");
        Serial.println(
            WiFi.disconnectReasonName(
                static_cast<wifi_err_reason_t>(motivo)
            )
        );
    }
}

void InicializarWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.onEvent(WiFiEvento);

    Serial.println("Iniciando conexao Wi-Fi...");
    Serial.print("Rede configurada: ");
    Serial.println(FLUXID_WIFI_SSID);

    WiFi.begin(FLUXID_WIFI_SSID, FLUXID_WIFI_PASSWORD);

    ultimaTentativaWiFi = millis();
}

void VerificarConexaoWiFi()
{
    uint32_t agora = millis();

    if (WiFi.status() == WL_CONNECTED)
    {
        if (!WiFiJaConectado)
        {
            WiFiJaConectado = true;

            Serial.println("Wi-Fi conectado!");
            Serial.print("IP do ESP32: ");
            Serial.println(WiFi.localIP());

            Serial.print("Intensidade do sinal (RSSI): ");
            Serial.println(WiFi.RSSI());
        }

        return;
    }

    WiFiJaConectado = false;

    if (agora - ultimaTentativaWiFi >= INTERVALO_RECONEXAO)
    {
        Serial.println("Tentando reconectar ao Wi-Fi...");

        WiFi.begin(FLUXID_WIFI_SSID, FLUXID_WIFI_PASSWORD);

        ultimaTentativaWiFi = millis();
    }
}