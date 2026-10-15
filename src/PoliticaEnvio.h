#pragma once

// Regras de decisao do envio de telemetria, sem dependencia do Arduino.
// Ficam isoladas para poderem ser testadas no computador (test/host).

#include <stdint.h>

#include <Estado.h>

// O que fazer com a mensagem depois da resposta da API
// (Guia-Firmware-Lacre-IoT, secoes 5, 6 e 7).
enum AcaoAposEnvio : uint8_t
{
    ENVIO_CONFIRMADO, // 200, 201, 202 ou 409: remover da fila
    ENVIO_REJEITADO,  // 400 ou 413: corpo invalido, descartar
    ENVIO_PAUSAR,     // 401, 403 ou 404: chave/cadastro; manter e nao insistir
    ENVIO_REPETIR     // 429, 5xx, outros codigos ou sem resposta: manter e reenviar
};

AcaoAposEnvio ClassificarRespostaHTTP(int codigo);

// Intervalo normal entre envios enquanto nao ha falhas.
const uint32_t INTERVALO_ENVIO_NORMAL_MS = 2000UL;

// Limite de seguranca para um Retry-After anormal. As janelas da API
// sao de 1 minuto, entao o Retry-After esperado fica abaixo disso.
const uint32_t ESPERA_MAXIMA_RETRY_AFTER_MS = 900000UL;

// Espera apos a N-esima falha temporaria seguida (N >= 1):
// 5 s, 15 s, 30 s e, dali em diante, 60 s.
uint32_t EsperaAposFalha(uint8_t falhasSeguidas);

// Espera ate a N-esima verificacao de autorizacao durante a pausa
// (N >= 0): 30 s, 60 s, 120 s e, dali em diante, 300 s.
uint32_t EsperaVerificacaoPausa(uint8_t verificacoesFeitas);

// Converte o valor do cabecalho Retry-After em segundos.
// Retorna 0 se vazio, invalido ou no formato de data HTTP.
uint32_t InterpretarRetryAfter(const char* valor);

// Se a linha de cabecalho for "Retry-After: N", grava N em segundos.
bool ExtrairRetryAfter(const char* linha, uint32_t& segundos);

enum AcaoNaFila : uint8_t
{
    FILA_REMOVER,   // entrega confirmada
    FILA_DESCARTAR, // corpo invalido: registrar e descartar
    FILA_MANTER     // continua na fila para nova tentativa
};

struct EstadoEnvio
{
    uint8_t falhasSeguidas;    // falhas temporarias seguidas
    uint8_t verificacoesPausa; // verificacoes feitas durante a pausa
    int codigoPausa;           // 401/403/404 que pausou; 0 = envio normal
    uint32_t intervaloMs;      // espera ate a proxima acao de envio
};

void IniciarEstadoEnvio(EstadoEnvio& estado);

struct DecisaoEnvio
{
    AcaoNaFila fila;
    bool registrarTentativa; // regravar a mensagem com attempt_count + 1
};

// Decide o destino da mensagem e a proxima espera apos um envio.
DecisaoEnvio DecidirAposEnvio(
    EstadoEnvio& estado,
    AcaoAposEnvio acao,
    int codigo,
    uint32_t retryAfterSegundos,
    bool requisicaoRealizada
);

// Durante a pausa (codigoPausa != 0), decide se a autorizacao voltou.
// So um 200 da verificacao retoma o envio normal; nada e apagado aqui.
void DecidirAposVerificacao(
    EstadoEnvio& estado,
    int codigo,
    uint32_t retryAfterSegundos
);

// Estado a mostrar nos LEDs: ERRO com a fila indisponivel, DESATIVADO
// com 403 e NAO_CADASTRADO com 401/404. So altera o estado se ele
// estiver OPERACIONAL ou se foi esta funcao que o alterou
// (sinalizacaoAtiva), para nao sobrescrever outros estados.
EstadoDispositivo CalcularEstadoSinalizado(
    EstadoDispositivo atual,
    bool& sinalizacaoAtiva,
    bool filaDisponivel,
    int codigoPausa
);
