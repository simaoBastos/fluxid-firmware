#include <Mensagem.h>
#include <Dispositivo.h>
#include <Preferences.h>

DadosMensagem dadosMensagem;

namespace
{
    Preferences preferencias;
    uint32_t numeroAtual = 0;

    void AtualizarIdentificador()
    {
        char buffer[64];

        snprintf(
            buffer,
            sizeof(buffer),
            "MSG-%s-%06lu",
            dadosDispositivo.device_id.c_str(),
            static_cast<unsigned long>(numeroAtual)
        );

        dadosMensagem.message_id = String(buffer);
    }
}

void InicializarMensagem()
{
    preferencias.begin("fluxid", false);

    numeroAtual = preferencias.getUInt("msg_num", 0);

    dadosMensagem.attempt_count = 0;
    AtualizarIdentificador();
}

void NovaMensagem()
{
    numeroAtual++;

    // Persiste o contador para evitar reutilizar IDs após reinicialização.
    preferencias.putUInt("msg_num", numeroAtual);

    dadosMensagem.attempt_count = 0;
    AtualizarIdentificador();
}