#pragma once

#include<Arduino.h>

struct DadosDispositivo
{
    String device_id;
    double battery_percent;
    String seal_status;
};

extern DadosDispositivo dadosDispositivo;


void InicializarDispositivo();
void ObterDadosDispositivo();