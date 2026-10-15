#pragma once

#include <Arduino.h>

bool InicializarFilaOffline();

bool AdicionarNaFilaOffline(const String& payload);

bool ObterProximaDaFilaOffline(
    String& caminho,
    String& payload
);

bool RemoverDaFilaOffline(const String& caminho);

// Remove uma mensagem rejeitada pela API (HTTP 400, 401, 403, 404 ou
// 413), registrando o codigo e o conteudo no serial.
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
