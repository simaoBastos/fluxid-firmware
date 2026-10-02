#pragma once

#include <Arduino.h>
#include <TinyGPS++.h>

struct DadosGPS /*struct provisória enquanto o contrato com o WebService não é definido. contém os principais dados do GPS; será ajustada após a definição do contrato.*/
{
    uint8_t satelites;
    double latitude;
    double longitude;
    double hdop;
};

extern DadosGPS dadosGPS;

void InicializarGPS(int8_t rxPin, int8_t txPin);
bool DadosGPSAtualizados();
bool LocalizacaoValida();
void ObterDadosGPS();
void LerGPS();
void ImprimirLocalizacaoInvalida();