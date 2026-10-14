#include <JSON.h>
#include <GPS.h>
#include <GSM.h>
#include <Dispositivo.h>
#include <Mensagem.h>

String MontarJsonTelemetria()
{
    // Não gera telemetria sem uma posição válida.
    if (!dadosTelemetria.tem_posicao_valida)
    {
        return "";
    }

    JsonDocument doc;

    doc["device_id"] = dadosDispositivo.device_id;
    doc["message_id"] = dadosMensagem.message_id;

    doc["latitude"] = dadosTelemetria.latitude;
    doc["longitude"] = dadosTelemetria.longitude;
    doc["gps_ok"] = dadosTelemetria.gps_ok;
    doc["satellites"] = dadosTelemetria.satellites;
    doc["hdop"] = dadosTelemetria.hdop;
    doc["speed_kmh"] = dadosTelemetria.speed_kmh;
    doc["attempt_count"] = dadosMensagem.attempt_count;

    // TESTE TEMPORÁRIO: a API exige este campo.
    // Não representa uma medição real da bateria.
    doc["battery_percent"] = 100;

    if (dadosDispositivo.seal_status == "LOCKED" ||
        dadosDispositivo.seal_status == "UNLOCKED" ||
        dadosDispositivo.seal_status == "BROKEN")
    {
        doc["seal_status"] = dadosDispositivo.seal_status;
    }

    if (dadosGSM.modem_respondendo)
    {
        doc["gsm_signal"] = dadosGSM.gsm_signal;
    }

    String dados;
    serializeJson(doc, dados);

    Serial.println("JSON da telemetria:");
    Serial.println(dados);

    return dados;
}