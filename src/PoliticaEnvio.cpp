#include <PoliticaEnvio.h>

namespace
{
    // Etapa de demonstracao: 5 s, 15 s, 30 s e teto de 60 s.
    const uint32_t ESPERAS_APOS_FALHA[] = {5000UL, 15000UL, 30000UL, 60000UL};
    const uint8_t QTD_ESPERAS =
        sizeof(ESPERAS_APOS_FALHA) / sizeof(ESPERAS_APOS_FALHA[0]);

    // Com 401, 403 ou 404 o Guia pede para nao insistir. A verificacao e
    // espacada; o limite da API e de 60 chaves erradas por minuto.
    const uint32_t ESPERAS_VERIFICACAO[] = {30000UL, 60000UL, 120000UL, 300000UL};
    const uint8_t QTD_VERIFICACOES =
        sizeof(ESPERAS_VERIFICACAO) / sizeof(ESPERAS_VERIFICACAO[0]);

    char Minuscula(char c)
    {
        return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    }

    bool Espaco(char c)
    {
        return c == ' ' || c == '\t';
    }

    // Espera para um 429: o Retry-After valido manda; ilegivel, espera
    // o teto da espera crescente; ausente, usa a espera padrao.
    uint32_t EsperaPara429(const RespostaAPI& resposta, uint32_t esperaPadrao)
    {
        if (resposta.retry_after_s > 0)
            return EsperaRetryAfterMs(resposta.retry_after_s);

        if (resposta.retry_after_invalido)
            return ESPERA_RETRY_AFTER_INVALIDO_MS;

        return esperaPadrao;
    }

    void SubirNivelPausa(EstadoEnvio& estado)
    {
        if (estado.nivelPausa < 0xFF)
            estado.nivelPausa++;
    }
}

AcaoAposEnvio ClassificarRespostaHTTP(int codigo)
{
    if (codigo == 200 || codigo == 201 ||
        codigo == 202 || codigo == 409)
        return ENVIO_CONFIRMADO;

    // Problema no proprio corpo: reenviar a mesma mensagem nao resolve.
    if (codigo == 400 || codigo == 413)
        return ENVIO_REJEITADO;

    // Chave ou cadastro: a secao 7 do Guia manda descartar a mensagem
    // rejeitada, e as secoes 5 e 6, nao insistir. Descarta so esta
    // mensagem e pausa o envio das demais.
    if (codigo == 401 || codigo == 403 || codigo == 404)
        return ENVIO_PAUSAR;

    // 429 (aguardar Retry-After), 5xx, codigos nao previstos no Guia
    // ou sem resposta (codigo negativo): manter e reenviar.
    return ENVIO_REPETIR;
}

uint32_t EsperaRetryAfterMs(uint32_t segundos)
{
    // Compara em segundos para nao estourar o uint32_t.
    return segundos > RETRY_AFTER_MAXIMO_S
               ? 0xFFFFFFFFUL
               : segundos * 1000UL;
}

uint32_t EsperaAposFalha(uint8_t falhasSeguidas)
{
    if (falhasSeguidas == 0)
        return INTERVALO_ENVIO_NORMAL_MS;

    uint8_t indice = falhasSeguidas > QTD_ESPERAS
                         ? QTD_ESPERAS - 1
                         : falhasSeguidas - 1;

    return ESPERAS_APOS_FALHA[indice];
}

uint32_t EsperaVerificacaoPausa(uint8_t nivelPausa)
{
    if (nivelPausa == 0)
        nivelPausa = 1;

    uint8_t indice = nivelPausa > QTD_VERIFICACOES
                         ? QTD_VERIFICACOES - 1
                         : nivelPausa - 1;

    return ESPERAS_VERIFICACAO[indice];
}

bool InterpretarRetryAfter(const char* valor, uint32_t& segundos)
{
    segundos = 0;

    if (valor == nullptr)
        return false;

    while (Espaco(*valor))
        valor++;

    if (*valor < '0' || *valor > '9')
        return false;

    uint32_t acumulado = 0;
    bool saturado = false;

    while (*valor >= '0' && *valor <= '9')
    {
        uint32_t digito = static_cast<uint32_t>(*valor - '0');

        // Satura em vez de estourar; o limite e aplicado depois.
        if (saturado || acumulado > (0xFFFFFFFFUL - digito) / 10UL)
            saturado = true;
        else
            acumulado = acumulado * 10UL + digito;

        valor++;
    }

    while (Espaco(*valor))
        valor++;

    // "120abc" ou uma data HTTP nao sao segundos validos.
    if (*valor != '\0')
        return false;

    segundos = saturado ? 0xFFFFFFFFUL : acumulado;
    return true;
}

bool ExtrairRetryAfter(const char* linha, uint32_t& segundos, bool& valido)
{
    const char* nome = "retry-after";

    segundos = 0;
    valido = false;

    if (linha == nullptr)
        return false;

    while (*nome != '\0')
    {
        if (Minuscula(*linha) != *nome)
            return false;

        linha++;
        nome++;
    }

    while (Espaco(*linha))
        linha++;

    if (*linha != ':')
        return false;

    valido = InterpretarRetryAfter(linha + 1, segundos);
    return true;
}

void IniciarEstadoEnvio(EstadoEnvio& estado)
{
    estado.falhasSeguidas = 0;
    estado.nivelPausa = 0;
    estado.codigoPausa = 0;
    estado.intervaloMs = INTERVALO_ENVIO_NORMAL_MS;
}

DecisaoEnvio DecidirAposEnvio(EstadoEnvio& estado, const RespostaAPI& resposta)
{
    DecisaoEnvio decisao;
    decisao.fila = FILA_MANTER;
    decisao.registrarTentativa = false;

    switch (ClassificarRespostaHTTP(resposta.codigo))
    {
        case ENVIO_CONFIRMADO:
            // Telemetria aceita: unica prova de que tudo voltou a funcionar.
            decisao.fila = FILA_REMOVER;
            IniciarEstadoEnvio(estado);
            break;

        case ENVIO_REJEITADO:
            decisao.fila = FILA_DESCARTAR;
            estado.falhasSeguidas = 0;
            estado.intervaloMs = INTERVALO_ENVIO_NORMAL_MS;
            break;

        case ENVIO_PAUSAR:
            // Secao 7: descarta so a mensagem rejeitada. As demais ficam
            // na fila ate a verificacao periodica liberar o envio.
            decisao.fila = FILA_DESCARTAR;
            SubirNivelPausa(estado);
            estado.codigoPausa = resposta.codigo;
            estado.falhasSeguidas = 0;
            estado.intervaloMs = EsperaVerificacaoPausa(estado.nivelPausa);
            break;

        case ENVIO_REPETIR:
            // Mesmo message_id; +1 so se uma tentativa foi transmitida.
            decisao.registrarTentativa = resposta.requisicao_realizada;

            if (estado.falhasSeguidas < 0xFF)
                estado.falhasSeguidas++;

            {
                uint32_t padrao = EsperaAposFalha(estado.falhasSeguidas);

                estado.intervaloMs =
                    resposta.codigo == 429
                        ? EsperaPara429(resposta, padrao)
                        : padrao;
            }
            break;
    }

    return decisao;
}

void DecidirAposVerificacao(EstadoEnvio& estado, const RespostaAPI& resposta)
{
    // GET /iot/commands/{device_id} responde 200 quando a chave e o
    // cadastro estao validos (Regras-de-Negocio-e-Banco-Oxide). Isso
    // libera uma nova telemetria, mas nao zera o nivel da pausa: se a
    // telemetria for rejeitada de novo, a proxima espera e maior.
    if (resposta.codigo == 200)
    {
        estado.codigoPausa = 0;
        estado.falhasSeguidas = 0;
        estado.intervaloMs = INTERVALO_ENVIO_NORMAL_MS;
        return;
    }

    if (resposta.codigo == 401 || resposta.codigo == 403 ||
        resposta.codigo == 404)
        estado.codigoPausa = resposta.codigo;

    SubirNivelPausa(estado);

    uint32_t padrao = EsperaVerificacaoPausa(estado.nivelPausa);

    estado.intervaloMs =
        resposta.codigo == 429 ? EsperaPara429(resposta, padrao) : padrao;
}

EstadoDispositivo CalcularEstadoSinalizado(
    EstadoDispositivo atual,
    bool& sinalizacaoAtiva,
    bool filaDisponivel,
    int codigoPausa)
{
    EstadoDispositivo desejado = OPERACIONAL;

    if (!filaDisponivel)
        desejado = ERRO;
    else if (codigoPausa == 403)
        desejado = DESATIVADO;
    else if (codigoPausa == 401 || codigoPausa == 404)
        desejado = NAO_CADASTRADO;

    if (desejado != OPERACIONAL)
    {
        if (atual == OPERACIONAL || sinalizacaoAtiva)
        {
            sinalizacaoAtiva = true;
            return desejado;
        }

        return atual;
    }

    if (sinalizacaoAtiva)
    {
        sinalizacaoAtiva = false;
        return OPERACIONAL;
    }

    return atual;
}
