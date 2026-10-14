#pragma once

#include <Arduino.h>

bool InicializarFilaOffline();

bool AdicionarNaFilaOffline(const String& payload);

bool ObterProximaDaFilaOffline(
    String& caminho,
    String& payload
);

bool RemoverDaFilaOffline(const String& caminho);

uint16_t QuantidadeNaFilaOffline();

bool LimparFilaOffline();
