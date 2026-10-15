#pragma once

// Regras de decisao do envio de telemetria, sem dependencia do Arduino.
// Ficam isoladas para poderem ser testadas no computador (test/host).

#include <stdint.h>

#include <Estado.h>

// O que fazer com a mensagem depois da resposta da API
// (Guia-Firmware-Lacre-IoT, secoes 6 e 7).
enum AcaoAposEnvio : uint8_t
{
    ENVIO_CONFIRMADO, // 200, 201, 202 ou 409: remover da fila
    ENVIO_REJEITADO,  // 400 ou 413: corpo invalido, descartar
    ENVIO_PAUSAR,     // 401, 403 ou 404: descartar esta mensagem e pausar
    ENVIO_REPETIR     // 429, 5xx, outros codigos ou sem resposta: manter e reenviar
};

AcaoAposEnvio ClassificarRespostaHTTP(int codigo);

// Resposta de uma requisicao a API, como o HTTP.cpp a entrega.
struct RespostaAPI
{
    int codigo;                // status HTTP, ou negativo sem resposta
    uint32_t retry_after_s;    // Retry-After valido, em segundos (0 se ausente)
    bool retry_after_invalido; // cabecalho presente, mas nao e um numero de segundos
    bool requisicao_realizada; // false se nada foi transmitido (sem Wi-Fi, URL invalida)
};

// Intervalo normal entre envios enquanto nao ha falhas.
const uint32_t INTERVALO_ENVIO_NORMAL_MS = 2000UL;

// Maior Retry-After que o temporizador de 32 bits em milissegundos
// consegue representar (~49,7 dias). Acima disso a espera e limitada
// a esse maximo, com aviso no serial.
const uint32_t RETRY_AFTER_MAXIMO_S = 0xFFFFFFFFUL / 1000UL;

// Retry-After acima disto recebe um aviso no serial (mas e respeitado).
const uint32_t RETRY_AFTER_LONGO_S = 900UL;

// Retry-After presente, mas ilegivel (ex.: data HTTP): espera o teto
// da espera crescente em vez de arriscar uma tentativa rapida.
const uint32_t ESPERA_RETRY_AFTER_INVALIDO_MS = 60000UL;

// Converte segundos de Retry-After em milissegundos, sem estourar.
uint32_t EsperaRetryAfterMs(uint32_t segundos);

// Espera apos a N-esima falha temporaria seguida (N >= 1):
// 5 s, 15 s, 30 s e, dali em diante, 60 s.
uint32_t EsperaAposFalha(uint8_t falhasSeguidas);

// Espera no N-esimo nivel de pausa por chave/cadastro (N >= 1):
// 30 s, 60 s, 120 s e, dali em diante, 300 s.
uint32_t EsperaVerificacaoPausa(uint8_t nivelPausa);

// Converte o valor do cabecalho Retry-After (segundos, RFC 9110).
// Retorna false se nao for um numero inteiro de segundos (ex.: data
// HTTP). Valores maiores que uint32_t saturam em 0xFFFFFFFF.
bool InterpretarRetryAfter(const char* valor, uint32_t& segundos);

// Se a linha for o cabecalho Retry-After, retorna true e informa o
// valor e se ele e valido.
bool ExtrairRetryAfter(const char* linha, uint32_t& segundos, bool& valido);

enum AcaoNaFila : uint8_t
{
    FILA_REMOVER,   // entrega confirmada
    FILA_DESCARTAR, // rejeitada (400/413/401/403/404): registrar e descartar so esta
    FILA_MANTER     // continua na fila para nova tentativa
};

struct EstadoEnvio
{
    uint8_t falhasSeguidas; // falhas temporarias seguidas
    // Rejeicoes por chave/cadastro e verificacoes sem sucesso desde a
    // ultima telemetria aceita. So zera quando a API aceita telemetria,
    // para um 200 da verificacao nao abrir um ciclo rapido.
    uint8_t nivelPausa;
    int codigoPausa;        // 401/403/404 que pausou; 0 = envio normal
    uint32_t intervaloMs;   // espera ate a proxima acao de envio
};

void IniciarEstadoEnvio(EstadoEnvio& estado);

struct DecisaoEnvio
{
    AcaoNaFila fila;
    bool registrarTentativa; // regravar a mensagem com attempt_count + 1
};

// Decide o destino da mensagem e a proxima espera apos um envio.
DecisaoEnvio DecidirAposEnvio(EstadoEnvio& estado, const RespostaAPI& resposta);

// Durante a pausa (codigoPausa != 0), decide se as telemetrias voltam a
// ser enviadas. Um 200 da verificacao retoma o envio, mas nao zera o
// nivel da pausa: so uma telemetria aceita prova a recuperacao.
void DecidirAposVerificacao(EstadoEnvio& estado, const RespostaAPI& resposta);

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
