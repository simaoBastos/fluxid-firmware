#pragma once

#include <Arduino.h>

bool InicializarFilaOffline();

bool AdicionarNaFilaOffline(const String& payload);

bool ObterProximaDaFilaOffline(
    String& caminho,
    String& payload
);

bool RemoverDaFilaOffline(const String& caminho);

// Remove uma mensagem com corpo invalido (HTTP 400 ou 413),
// registrando o conteudo no serial.
bool DescartarDaFilaOffline(
    const String& caminho,
    const String& payload,
    int codigoHTTP
);

// Regrava a mensagem com attempt_count + 1 antes do proximo reenvio.
bool RegistrarTentativaNaFilaOffline(
    const String& caminho,
    const String& payload
);

uint16_t QuantidadeNaFilaOffline();

bool LimparFilaOffline();
