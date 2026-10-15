#pragma once

#include <Arduino.h>

#include <PoliticaEnvio.h>

// Resposta da API (PoliticaEnvio.h) e a acao correspondente.
struct ResultadoEnvio : RespostaAPI
{
    AcaoAposEnvio acao;
};

ResultadoEnvio EnviarDadosTelemetria(const String& payload);

// GET /iot/commands/{device_id}: confirma se a chave e o cadastro
// voltaram a ser aceitos, sem reenviar telemetria (usado na pausa).
ResultadoEnvio VerificarAutorizacaoAPI();
