#pragma once

#include <Arduino.h>

struct DadosMensagem{
    String message_id;
    uint32_t attempt_count;
};
extern DadosMensagem dadosMensagem;

void InicializarMensagem();
void NovaMensagem();