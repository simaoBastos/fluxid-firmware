#include <JSON.h>
#include <GPS.h>
#include <GSM.h>
#include <Dispositivo.h>
#include <Mensagem.h>

String MontarJsonTelemetria()
{
    // Não gera uma telemetria sem nunca ter obtido uma posição válida.
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

    // Só inclui valores de bateria dentro da faixa válida.
    if (dadosDispositivo.battery_percent >= 0 &&
        dadosDispositivo.battery_percent <= 100)
    {
        doc["battery_percent"] = dadosDispositivo.battery_percent;
    }

    // Não declara um estado físico que ainda não foi confirmado.
    if (dadosDispositivo.seal_status == "LOCKED" ||
        dadosDispositivo.seal_status == "UNLOCKED" ||
        dadosDispositivo.seal_status == "BROKEN")
    {
        doc["seal_status"] = dadosDispositivo.seal_status;
    }

    // Só informa sinal GSM se o modem respondeu.
    if (dadosGSM.modem_respondendo)
    {
        doc["gsm_signal"] = dadosGSM.gsm_signal;
    }

    String dados;
    serializeJson(doc, dados);
    return dados;
}