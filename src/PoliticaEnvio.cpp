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

    // Espera pedida pelo servidor, limitada pelo teto de seguranca.
    // Compara em segundos para nao estourar o uint32_t.
    uint32_t EsperaRetryAfter(uint32_t segundos)
    {
        return segundos > ESPERA_MAXIMA_RETRY_AFTER_MS / 1000UL
                   ? ESPERA_MAXIMA_RETRY_AFTER_MS
                   : segundos * 1000UL;
    }

    char Minuscula(char c)
    {
        return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
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

    // Chave ou cadastro: o problema nao e da mensagem. Ela e preservada
    // e o envio e pausado para nao insistir (secoes 5 e 6 do Guia).
    if (codigo == 401 || codigo == 403 || codigo == 404)
        return ENVIO_PAUSAR;

    // 429 (aguardar Retry-After), 5xx, codigos nao previstos no Guia
    // ou sem resposta (codigo negativo): manter e reenviar.
    return ENVIO_REPETIR;
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

uint32_t EsperaVerificacaoPausa(uint8_t verificacoesFeitas)
{
    uint8_t indice = verificacoesFeitas >= QTD_VERIFICACOES
                         ? QTD_VERIFICACOES - 1
                         : verificacoesFeitas;

    return ESPERAS_VERIFICACAO[indice];
}

uint32_t InterpretarRetryAfter(const char* valor)
{
    if (valor == nullptr)
        return 0;

    while (*valor == ' ' || *valor == '\t')
        valor++;

    if (*valor < '0' || *valor > '9')
        return 0;

    uint32_t segundos = 0;

    while (*valor >= '0' && *valor <= '9')
    {
        uint32_t digito = static_cast<uint32_t>(*valor - '0');

        // Satura em vez de estourar; o teto e aplicado depois.
        if (segundos > (0xFFFFFFFFUL - digito) / 10UL)
            return 0xFFFFFFFFUL;

        segundos = segundos * 10UL + digito;
        valor++;
    }

    while (*valor == ' ' || *valor == '\t')
        valor++;

    // "120abc" ou uma data HTTP nao sao segundos validos.
    return *valor == '\0' ? segundos : 0;
}

bool ExtrairRetryAfter(const char* linha, uint32_t& segundos)
{
    const char* nome = "retry-after";

    if (linha == nullptr)
        return false;

    while (*nome != '\0')
    {
        if (Minuscula(*linha) != *nome)
            return false;

        linha++;
        nome++;
    }

    while (*linha == ' ' || *linha == '\t')
        linha++;

    if (*linha != ':')
        return false;

    segundos = InterpretarRetryAfter(linha + 1);
    return true;
}

void IniciarEstadoEnvio(EstadoEnvio& estado)
{
    estado.falhasSeguidas = 0;
    estado.verificacoesPausa = 0;
    estado.codigoPausa = 0;
    estado.intervaloMs = INTERVALO_ENVIO_NORMAL_MS;
}

DecisaoEnvio DecidirAposEnvio(
    EstadoEnvio& estado,
    AcaoAposEnvio acao,
    int codigo,
    uint32_t retryAfterSegundos,
    bool requisicaoRealizada)
{
    DecisaoEnvio decisao;
    decisao.fila = FILA_MANTER;
    decisao.registrarTentativa = false;

    switch (acao)
    {
        case ENVIO_CONFIRMADO:
            decisao.fila = FILA_REMOVER;
            IniciarEstadoEnvio(estado);
            break;

        case ENVIO_REJEITADO:
            decisao.fila = FILA_DESCARTAR;
            estado.falhasSeguidas = 0;
            estado.intervaloMs = INTERVALO_ENVIO_NORMAL_MS;
            break;

        case ENVIO_PAUSAR:
            // Mesmo message_id; +1 porque uma tentativa foi realizada.
            decisao.registrarTentativa = requisicaoRealizada;
            estado.codigoPausa = codigo;
            estado.verificacoesPausa = 0;
            estado.falhasSeguidas = 0;
            estado.intervaloMs = EsperaVerificacaoPausa(0);
            break;

        case ENVIO_REPETIR:
            decisao.registrarTentativa = requisicaoRealizada;

            if (estado.falhasSeguidas < 0xFF)
                estado.falhasSeguidas++;

            estado.intervaloMs =
                (codigo == 429 && retryAfterSegundos > 0)
                    ? EsperaRetryAfter(retryAfterSegundos)
                    : EsperaAposFalha(estado.falhasSeguidas);
            break;
    }

    return decisao;
}

void DecidirAposVerificacao(
    EstadoEnvio& estado,
    int codigo,
    uint32_t retryAfterSegundos)
{
    // GET /iot/commands/{device_id} responde 200 quando a chave e o
    // cadastro estao validos (Regras-de-Negocio-e-Banco-Oxide).
    if (codigo == 200)
    {
        IniciarEstadoEnvio(estado);
        return;
    }

    if (codigo == 401 || codigo == 403 || codigo == 404)
        estado.codigoPausa = codigo;

    if (estado.verificacoesPausa < 0xFF)
        estado.verificacoesPausa++;

    estado.intervaloMs =
        (codigo == 429 && retryAfterSegundos > 0)
            ? EsperaRetryAfter(retryAfterSegundos)
            : EsperaVerificacaoPausa(estado.verificacoesPausa);
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
