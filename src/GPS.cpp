#include <GPS.h>

HardwareSerial SerialGPS(1);
TinyGPSPlus GPS;

DadosTelemetria dadosTelemetria;

void InicializarGPS(int8_t pinoRX, int8_t pinoTX)
{
    SerialGPS.begin(9600, SERIAL_8N1, pinoRX, pinoTX);
}

bool DadosGPSAtualizados()
{
    // Mantemos o gatilho original do projeto.
    return GPS.satellites.isUpdated();
}

bool LocalizacaoValida()
{
    return dadosTelemetria.tem_posicao_valida;
}

void ObterDadosGPS()
{
    dadosTelemetria.satellites = GPS.satellites.value();
    dadosTelemetria.hdop = GPS.hdop.hdop();

    // Só atualiza as coordenadas se a posição for válida
    // e tiver sido recebida recentemente.
    if (GPS.location.isValid() && GPS.location.age() <= 5000)
    {
        dadosTelemetria.latitude = GPS.location.lat();
        dadosTelemetria.longitude = GPS.location.lng();
        dadosTelemetria.speed_kmh = GPS.speed.kmph();

        dadosTelemetria.gps_ok = true;
        dadosTelemetria.tem_posicao_valida = true;
    }
    else
    {
        // Preserva a última posição conhecida.
        dadosTelemetria.gps_ok = false;
    }
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
    Serial.println("Aguardando primeira posicao GPS valida...");
    Serial.println("--------------------");
}