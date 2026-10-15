#pragma once

// Arduino.h simulado para compilar src/LED.cpp no computador.
// Guarda o nivel de cada pino e controla o millis().

#include <stdint.h>

#define HIGH 1
#define LOW 0
#define OUTPUT 1

namespace mock
{
    inline uint32_t& Relogio()
    {
        static uint32_t relogio = 0;
        return relogio;
    }

    inline int* Pinos()
    {
        static int pinos[32] = {0};
        return pinos;
    }

    inline int& Escritas()
    {
        static int escritas = 0;
        return escritas;
    }
}

inline uint32_t millis()
{
    return mock::Relogio();
}

inline void pinMode(uint8_t, uint8_t) {}

inline void digitalWrite(uint8_t pino, uint8_t valor)
{
    mock::Pinos()[pino] = valor ? HIGH : LOW;
    mock::Escritas()++;
}
