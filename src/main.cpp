#include <GPS.h>
#include <GSM.h>
#include <LED.h>
#include <HTTP.h>
#include <JSON.h>
#include <Arduino.h>
#include <WiFiConn.h>
#include <Dispositivo.h>
#include <Mensagem.h>

const bool ENVIO_HABILITADO = false;
const uint16_t INTERVALO_ENVIO_TELEMETRIA = 30000;

uint32_t ultimoEnvioTelemetria = 0;

void setup()
{
    Serial.begin(115200);

    InicializarWiFi();
    InicializarLEDs(0, 1);
    InicializarGPS(20, 21);
    InicializarGSM(6, 7);
    InicializarDispositivo();
    InicializarMensagem();
}

void loop()
{
    VerificarConexaoWiFi();
    LerGPS();
    AtualizarGSM();
    AtualizarLEDs();

    static uint32_t ultimoDiagnostico = 0;

    if (millis() - ultimoDiagnostico >= 3000)
    {
        ultimoDiagnostico = millis();

        Serial.print("GSM: ");
        Serial.println(TextoEstadoGSM());

        Serial.print("Wi-Fi: ");
        Serial.println(
            WiFi.status() == WL_CONNECTED
                ? "Conectado"
                : "Desconectado"
        );

        Serial.print("Satelites: ");
        Serial.println(dadosTelemetria.satellites);

        Serial.print("Posicao conhecida: ");
        Serial.println(LocalizacaoValida() ? "Sim" : "Nao");

        Serial.print("Posicao GPS atual: ");
        Serial.println(dadosTelemetria.gps_ok ? "Valida" : "Indisponivel");

        Serial.print("Bateria medida: ");
        Serial.println(
            dadosDispositivo.battery_percent >= 0 &&
            dadosDispositivo.battery_percent <= 100
                ? "Sim"
                : "Nao"
        );
    }

    if (DadosGPSAtualizados())
    {
        ObterDadosGPS();

        if (LocalizacaoValida())
        {
            ObterDadosGSM();
            ObterDadosDispositivo();

            Serial.println("JSON preparado:");
            Serial.println(MontarJsonTelemetria());
        }
        else
        {
            ImprimirLocalizacaoInvalida();
        }
    }

    // O agendamento não depende de uma nova atualização de satélites.
    uint32_t agora = millis();

    bool bateriaValida =
        dadosDispositivo.battery_percent >= 0 &&
        dadosDispositivo.battery_percent <= 100;

    if (ENVIO_HABILITADO &&
        dadosTelemetria.tem_posicao_valida &&
        bateriaValida &&
        // (
        // WiFi.status() == WL_CONNECTED || // comentado até que o gsm seja responsãvel por fazer a conexão
        // GPRSConectado()
        // ) &&
        WiFi.status() == WL_CONNECTED &&
        agora - ultimoEnvioTelemetria >= INTERVALO_ENVIO_TELEMETRIA)
    {
        ultimoEnvioTelemetria = agora;

        NovaMensagem();
        EnviarDadosTelemetria();
    }
}