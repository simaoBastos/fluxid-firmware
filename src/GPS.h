#pragma once

#include <Arduino.h>
#include <TinyGPS++.h>


extern TinyGPSPlus GPS;

struct DadosTelemetria /* contém os principais dados do GPS para a telemetria */
{
    uint8_t satellites;
    double latitude;
    double longitude;
    double speed_kmh;
    double hdop;
    bool gps_ok;           // true = posição atual válida; false = posição preservada (última válida)
    bool tem_posicao_valida; // true após o primeiro fix válido; false antes do primeiro fix
};

extern DadosTelemetria dadosTelemetria;

void InicializarGPS(int8_t rxPin, int8_t txPin);
bool DadosGPSAtualizados();
bool LocalizacaoValida();
void ObterDadosGPS();
void LerGPS();
void ImprimirLocalizacaoInvalida();