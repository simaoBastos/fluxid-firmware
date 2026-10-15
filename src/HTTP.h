#pragma once

#include <Arduino.h>

#include <PoliticaEnvio.h>

struct ResultadoEnvio
{
    int codigo;                  // status HTTP, ou negativo sem resposta
    uint32_t retry_after_s;      // valor do cabecalho Retry-After (0 se ausente)
    bool requisicao_realizada;   // false se nada foi transmitido (sem Wi-Fi, URL invalida)
    AcaoAposEnvio acao;
};

ResultadoEnvio EnviarDadosTelemetria(const String& payload);

// GET /iot/commands/{device_id}: confirma se a chave e o cadastro
// voltaram a ser aceitos, sem reenviar telemetria (usado na pausa).
ResultadoEnvio VerificarAutorizacaoAPI();
