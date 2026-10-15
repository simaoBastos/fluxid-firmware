// Testes de src/PoliticaEnvio.cpp: classificacao HTTP, esperas,
// Retry-After, destino da mensagem na fila, pausa e sinalizacao.

#include "teste.h"

#include <PoliticaEnvio.h>

namespace
{
    RespostaAPI Resposta(
        int codigo,
        uint32_t retryAfter = 0,
        bool retryAfterInvalido = false,
        bool realizada = true)
    {
        RespostaAPI r;
        r.codigo = codigo;
        r.retry_after_s = retryAfter;
        r.retry_after_invalido = retryAfterInvalido;
        r.requisicao_realizada = realizada;
        return r;
    }

    // Simula um envio e devolve a decisao, como o main.cpp faz.
    DecisaoEnvio Enviar(
        EstadoEnvio& estado,
        int codigo,
        uint32_t retryAfter = 0,
        bool realizada = true)
    {
        return DecidirAposEnvio(
            estado, Resposta(codigo, retryAfter, false, realizada));
    }

    void Verificar(EstadoEnvio& estado, int codigo, uint32_t retryAfter = 0)
    {
        DecidirAposVerificacao(estado, Resposta(codigo, retryAfter));
    }

    void TestarClassificacao()
    {
        VERIFICAR_IGUAL(ENVIO_CONFIRMADO, ClassificarRespostaHTTP(200));
        VERIFICAR_IGUAL(ENVIO_CONFIRMADO, ClassificarRespostaHTTP(201));
        VERIFICAR_IGUAL(ENVIO_CONFIRMADO, ClassificarRespostaHTTP(202));
        VERIFICAR_IGUAL(ENVIO_CONFIRMADO, ClassificarRespostaHTTP(409));

        VERIFICAR_IGUAL(ENVIO_REJEITADO, ClassificarRespostaHTTP(400));
        VERIFICAR_IGUAL(ENVIO_REJEITADO, ClassificarRespostaHTTP(413));

        VERIFICAR_IGUAL(ENVIO_PAUSAR, ClassificarRespostaHTTP(401));
        VERIFICAR_IGUAL(ENVIO_PAUSAR, ClassificarRespostaHTTP(403));
        VERIFICAR_IGUAL(ENVIO_PAUSAR, ClassificarRespostaHTTP(404));

        VERIFICAR_IGUAL(ENVIO_REPETIR, ClassificarRespostaHTTP(429));
        VERIFICAR_IGUAL(ENVIO_REPETIR, ClassificarRespostaHTTP(500));
        VERIFICAR_IGUAL(ENVIO_REPETIR, ClassificarRespostaHTTP(503));

        // Codigos de erro do ArduinoHttpClient (sem resposta HTTP).
        VERIFICAR_IGUAL(ENVIO_REPETIR, ClassificarRespostaHTTP(-1)); // conexao
        VERIFICAR_IGUAL(ENVIO_REPETIR, ClassificarRespostaHTTP(-2)); // API
        VERIFICAR_IGUAL(ENVIO_REPETIR, ClassificarRespostaHTTP(-3)); // timeout
        VERIFICAR_IGUAL(ENVIO_REPETIR, ClassificarRespostaHTTP(-4)); // resposta invalida

        // Nao previstos no Guia: nunca sucesso nem descarte.
        VERIFICAR_IGUAL(ENVIO_REPETIR, ClassificarRespostaHTTP(204));
        VERIFICAR_IGUAL(ENVIO_REPETIR, ClassificarRespostaHTTP(301));
        VERIFICAR_IGUAL(ENVIO_REPETIR, ClassificarRespostaHTTP(405));
        VERIFICAR_IGUAL(ENVIO_REPETIR, ClassificarRespostaHTTP(422));
        VERIFICAR_IGUAL(ENVIO_REPETIR, ClassificarRespostaHTTP(0));
    }

    void TestarEsperas()
    {
        VERIFICAR_IGUAL(2000, EsperaAposFalha(0));
        VERIFICAR_IGUAL(5000, EsperaAposFalha(1));
        VERIFICAR_IGUAL(15000, EsperaAposFalha(2));
        VERIFICAR_IGUAL(30000, EsperaAposFalha(3));
        VERIFICAR_IGUAL(60000, EsperaAposFalha(4));
        VERIFICAR_IGUAL(60000, EsperaAposFalha(5));
        VERIFICAR_IGUAL(60000, EsperaAposFalha(255));

        VERIFICAR_IGUAL(30000, EsperaVerificacaoPausa(0));
        VERIFICAR_IGUAL(30000, EsperaVerificacaoPausa(1));
        VERIFICAR_IGUAL(60000, EsperaVerificacaoPausa(2));
        VERIFICAR_IGUAL(120000, EsperaVerificacaoPausa(3));
        VERIFICAR_IGUAL(300000, EsperaVerificacaoPausa(4));
        VERIFICAR_IGUAL(300000, EsperaVerificacaoPausa(255));
    }

    void TestarSequenciaDeFalhas()
    {
        EstadoEnvio estado;
        IniciarEstadoEnvio(estado);

        const uint32_t esperado[] = {5000, 15000, 30000, 60000, 60000, 60000};

        for (uint32_t espera : esperado)
        {
            DecisaoEnvio decisao = Enviar(estado, 500);

            VERIFICAR_IGUAL(FILA_MANTER, decisao.fila);
            VERIFICAR(decisao.registrarTentativa);
            VERIFICAR_IGUAL(espera, estado.intervaloMs);
            // Nunca ciclo rapido: toda falha espera pelo menos 5 s.
            VERIFICAR(estado.intervaloMs >= 5000);
        }

        // Muitas falhas nao estouram o contador.
        for (int i = 0; i < 300; i++)
            Enviar(estado, -3);

        VERIFICAR_IGUAL(60000, estado.intervaloMs);

        // Um sucesso volta ao intervalo normal e reinicia a sequencia.
        DecisaoEnvio decisao = Enviar(estado, 202);
        VERIFICAR_IGUAL(FILA_REMOVER, decisao.fila);
        VERIFICAR(!decisao.registrarTentativa);
        VERIFICAR_IGUAL(2000, estado.intervaloMs);

        Enviar(estado, 500);
        VERIFICAR_IGUAL(5000, estado.intervaloMs);
    }

    void TestarAttemptCount()
    {
        EstadoEnvio estado;
        IniciarEstadoEnvio(estado);

        // Sem transmissao (Wi-Fi caiu, URL invalida): nao conta tentativa.
        DecisaoEnvio decisao = Enviar(estado, -1, 0, false);
        VERIFICAR_IGUAL(FILA_MANTER, decisao.fila);
        VERIFICAR(!decisao.registrarTentativa);

        // Tentativas realizadas em que a mensagem fica: +1 cada.
        VERIFICAR(Enviar(estado, 503).registrarTentativa);
        VERIFICAR(Enviar(estado, 429, 10).registrarTentativa);

        // Mensagem que sai da fila nao e regravada.
        IniciarEstadoEnvio(estado);
        VERIFICAR(!Enviar(estado, 409).registrarTentativa);
        VERIFICAR(!Enviar(estado, 400).registrarTentativa);
        VERIFICAR(!Enviar(estado, 401).registrarTentativa);
    }

    void TestarRejeicao()
    {
        EstadoEnvio estado;
        IniciarEstadoEnvio(estado);

        Enviar(estado, 500);
        Enviar(estado, 500);

        // 400/413 descartam so esta mensagem e liberam a fila.
        DecisaoEnvio decisao = Enviar(estado, 400);
        VERIFICAR_IGUAL(FILA_DESCARTAR, decisao.fila);
        VERIFICAR_IGUAL(2000, estado.intervaloMs);
        VERIFICAR_IGUAL(0, estado.falhasSeguidas);
        VERIFICAR_IGUAL(0, estado.codigoPausa);

        VERIFICAR_IGUAL(FILA_DESCARTAR, Enviar(estado, 413).fila);
    }

    void TestarInterpretarRetryAfter()
    {
        uint32_t s = 99;

        VERIFICAR(InterpretarRetryAfter("120", s));
        VERIFICAR_IGUAL(120, s);
        VERIFICAR(InterpretarRetryAfter("  30 \t", s));
        VERIFICAR_IGUAL(30, s);
        VERIFICAR(InterpretarRetryAfter("0", s));
        VERIFICAR_IGUAL(0, s);
        VERIFICAR(InterpretarRetryAfter("3600", s));
        VERIFICAR_IGUAL(3600, s);
        VERIFICAR(InterpretarRetryAfter("4294967295", s));
        VERIFICAR_IGUAL(4294967295ULL, s);

        // Acima de uint32_t: satura, sem estourar para um valor pequeno.
        VERIFICAR(InterpretarRetryAfter("4294967296", s));
        VERIFICAR_IGUAL(0xFFFFFFFFUL, s);
        VERIFICAR(InterpretarRetryAfter("99999999999999999999", s));
        VERIFICAR_IGUAL(0xFFFFFFFFUL, s);

        // Invalidos: nao sao segundos.
        VERIFICAR(!InterpretarRetryAfter("", s));
        VERIFICAR_IGUAL(0, s);
        VERIFICAR(!InterpretarRetryAfter("   ", s));
        VERIFICAR(!InterpretarRetryAfter(nullptr, s));
        VERIFICAR(!InterpretarRetryAfter("-5", s));
        VERIFICAR(!InterpretarRetryAfter("+5", s));
        VERIFICAR(!InterpretarRetryAfter("12abc", s));
        VERIFICAR(!InterpretarRetryAfter("1.5", s));
        VERIFICAR(!InterpretarRetryAfter("12 34", s));
        VERIFICAR(!InterpretarRetryAfter("Wed, 21 Oct 2026 07:28:00 GMT", s));

        bool valido = false;
        VERIFICAR(ExtrairRetryAfter("Retry-After: 45", s, valido));
        VERIFICAR(valido);
        VERIFICAR_IGUAL(45, s);
        VERIFICAR(ExtrairRetryAfter("retry-after:7", s, valido));
        VERIFICAR_IGUAL(7, s);
        VERIFICAR(ExtrairRetryAfter("RETRY-AFTER : 9", s, valido));
        VERIFICAR_IGUAL(9, s);

        // Cabecalho presente com valor ilegivel: reconhecido e invalido.
        VERIFICAR(ExtrairRetryAfter("Retry-After: Wed, 21 Oct 2026", s, valido));
        VERIFICAR(!valido);
        VERIFICAR_IGUAL(0, s);
        VERIFICAR(ExtrairRetryAfter("Retry-After:", s, valido));
        VERIFICAR(!valido);

        VERIFICAR(!ExtrairRetryAfter("Content-Length: 12", s, valido));
        VERIFICAR(!ExtrairRetryAfter("Retry-After-Extra: 3", s, valido));
        VERIFICAR(!ExtrairRetryAfter("X-Retry-After: 3", s, valido));
        VERIFICAR(!ExtrairRetryAfter("", s, valido));
    }

    void TestarRetryAfter429()
    {
        EstadoEnvio estado;
        IniciarEstadoEnvio(estado);

        // 429 respeita o Retry-After do servidor.
        DecisaoEnvio decisao = Enviar(estado, 429, 45);
        VERIFICAR_IGUAL(FILA_MANTER, decisao.fila);
        VERIFICAR_IGUAL(45000, estado.intervaloMs);

        // Acima de 15 min: respeitado integralmente (sem reducao).
        Enviar(estado, 429, 901);
        VERIFICAR_IGUAL(901000, estado.intervaloMs);
        Enviar(estado, 429, 3600);
        VERIFICAR_IGUAL(3600000, estado.intervaloMs);
        Enviar(estado, 429, 86400);
        VERIFICAR_IGUAL(86400000ULL, estado.intervaloMs);

        // Maior valor representavel em ms de 32 bits, exato.
        Enviar(estado, 429, RETRY_AFTER_MAXIMO_S);
        VERIFICAR_IGUAL(RETRY_AFTER_MAXIMO_S * 1000ULL, estado.intervaloMs);

        // Acima disso: o maximo do temporizador, nunca um valor pequeno.
        Enviar(estado, 429, RETRY_AFTER_MAXIMO_S + 1);
        VERIFICAR_IGUAL(0xFFFFFFFFUL, estado.intervaloMs);
        Enviar(estado, 429, 0xFFFFFFFFUL);
        VERIFICAR_IGUAL(0xFFFFFFFFUL, estado.intervaloMs);

        // Sem Retry-After: espera crescente.
        IniciarEstadoEnvio(estado);
        Enviar(estado, 429, 0);
        VERIFICAR_IGUAL(5000, estado.intervaloMs);

        // Retry-After ilegivel: 60 s, nao a primeira espera curta.
        IniciarEstadoEnvio(estado);
        DecidirAposEnvio(estado, Resposta(429, 0, true));
        VERIFICAR_IGUAL(ESPERA_RETRY_AFTER_INVALIDO_MS, estado.intervaloMs);
        VERIFICAR(estado.intervaloMs >= 60000);

        // Retry-After so vale para 429.
        IniciarEstadoEnvio(estado);
        Enviar(estado, 503, 3600);
        VERIFICAR_IGUAL(5000, estado.intervaloMs);

        // Conversao direta, sem estouro.
        VERIFICAR_IGUAL(0, EsperaRetryAfterMs(0));
        VERIFICAR_IGUAL(1000, EsperaRetryAfterMs(1));
        VERIFICAR_IGUAL(4294967000ULL, EsperaRetryAfterMs(4294967));
        VERIFICAR_IGUAL(0xFFFFFFFFUL, EsperaRetryAfterMs(4294968));
    }

    void TestarPausaDescartaSoARejeitada()
    {
        EstadoEnvio estado;
        IniciarEstadoEnvio(estado);

        // 401: secao 7, descarta so esta mensagem; pausa as demais.
        DecisaoEnvio decisao = Enviar(estado, 401);
        VERIFICAR_IGUAL(FILA_DESCARTAR, decisao.fila);
        VERIFICAR(!decisao.registrarTentativa);
        VERIFICAR_IGUAL(401, estado.codigoPausa);
        VERIFICAR_IGUAL(1, estado.nivelPausa);
        VERIFICAR_IGUAL(30000, estado.intervaloMs);

        VERIFICAR_IGUAL(FILA_DESCARTAR, Enviar(estado, 403).fila);
        VERIFICAR_IGUAL(FILA_DESCARTAR, Enviar(estado, 404).fila);
    }

    void TestarVerificacaoNaPausa()
    {
        EstadoEnvio estado;
        IniciarEstadoEnvio(estado);
        Enviar(estado, 401);

        // Verificacoes sem sucesso espacam ate 5 min, sem apagar nada.
        Verificar(estado, 401);
        VERIFICAR_IGUAL(60000, estado.intervaloMs);
        Verificar(estado, -3); // rede: continua pausado
        VERIFICAR_IGUAL(401, estado.codigoPausa);
        VERIFICAR_IGUAL(120000, estado.intervaloMs);
        Verificar(estado, 403); // motivo atualizado
        VERIFICAR_IGUAL(403, estado.codigoPausa);
        VERIFICAR_IGUAL(300000, estado.intervaloMs);
        Verificar(estado, 500);
        VERIFICAR_IGUAL(300000, estado.intervaloMs);
        VERIFICAR(estado.intervaloMs >= 30000);

        // 429 na verificacao respeita o Retry-After, inclusive longo.
        Verificar(estado, 429, 50);
        VERIFICAR_IGUAL(50000, estado.intervaloMs);
        Verificar(estado, 429, 7200);
        VERIFICAR_IGUAL(7200000, estado.intervaloMs);
        VERIFICAR(estado.codigoPausa != 0);

        // Outros 2xx nao bastam como prova de autorizacao.
        Verificar(estado, 202);
        VERIFICAR(estado.codigoPausa != 0);

        // 200 libera o envio, mas mantem o nivel da pausa.
        uint8_t nivel = estado.nivelPausa;
        Verificar(estado, 200);
        VERIFICAR_IGUAL(0, estado.codigoPausa);
        VERIFICAR_IGUAL(2000, estado.intervaloMs);
        VERIFICAR_IGUAL(nivel, estado.nivelPausa);

        // Telemetria aceita: so entao a pausa e encerrada de vez.
        Enviar(estado, 202);
        VERIFICAR_IGUAL(0, estado.nivelPausa);

        Enviar(estado, 404);
        VERIFICAR_IGUAL(1, estado.nivelPausa);
        VERIFICAR_IGUAL(30000, estado.intervaloMs);
    }

    void TestarSemCicloRapidoComVerificacaoEnganosa()
    {
        // A verificacao responde 200, mas a telemetria continua 401.
        // Cada volta deve esperar mais, ate 5 min, e descartar no maximo
        // uma mensagem por volta.
        EstadoEnvio estado;
        IniciarEstadoEnvio(estado);

        uint32_t esperaAnterior = 0;
        int descartadas = 0;

        for (int volta = 0; volta < 20; volta++)
        {
            DecisaoEnvio decisao = Enviar(estado, 401);
            if (decisao.fila == FILA_DESCARTAR) descartadas++;

            uint32_t espera = estado.intervaloMs;
            VERIFICAR(espera >= 30000);
            VERIFICAR(espera >= esperaAnterior);
            esperaAnterior = espera;

            Verificar(estado, 200);
            VERIFICAR_IGUAL(0, estado.codigoPausa);
        }

        VERIFICAR_IGUAL(20, descartadas);
        // Depois de algumas voltas: so uma tentativa a cada 5 min.
        Enviar(estado, 401);
        VERIFICAR_IGUAL(300000, estado.intervaloMs);

        // Muitas voltas nao estouram o nivel.
        for (int i = 0; i < 300; i++)
            Enviar(estado, 403);
        VERIFICAR_IGUAL(0xFF, estado.nivelPausa);
        VERIFICAR_IGUAL(300000, estado.intervaloMs);
    }

    void TestarOutrasRespostasNaoEncerramPausa()
    {
        // Durante a pausa, 400/5xx de uma telemetria liberada nao zeram o
        // nivel: so 2xx/409 provam a recuperacao.
        EstadoEnvio estado;
        IniciarEstadoEnvio(estado);
        Enviar(estado, 401);
        Verificar(estado, 200);

        Enviar(estado, 400);
        VERIFICAR_IGUAL(1, estado.nivelPausa);
        Enviar(estado, 500);
        VERIFICAR_IGUAL(1, estado.nivelPausa);
        Enviar(estado, 401);
        VERIFICAR_IGUAL(2, estado.nivelPausa);
        VERIFICAR_IGUAL(60000, estado.intervaloMs);
    }

    void TestarSinalizacao()
    {
        bool ativa = false;

        // Fila indisponivel -> ERRO; recuperada -> OPERACIONAL.
        EstadoDispositivo estado =
            CalcularEstadoSinalizado(OPERACIONAL, ativa, false, 0);
        VERIFICAR_IGUAL(ERRO, estado);
        VERIFICAR(ativa);

        estado = CalcularEstadoSinalizado(estado, ativa, true, 0);
        VERIFICAR_IGUAL(OPERACIONAL, estado);
        VERIFICAR(!ativa);

        estado = CalcularEstadoSinalizado(estado, ativa, true, 401);
        VERIFICAR_IGUAL(NAO_CADASTRADO, estado);
        estado = CalcularEstadoSinalizado(estado, ativa, true, 404);
        VERIFICAR_IGUAL(NAO_CADASTRADO, estado);
        estado = CalcularEstadoSinalizado(estado, ativa, true, 403);
        VERIFICAR_IGUAL(DESATIVADO, estado);

        // A fila indisponivel tem prioridade sobre a pausa.
        estado = CalcularEstadoSinalizado(estado, ativa, false, 403);
        VERIFICAR_IGUAL(ERRO, estado);

        estado = CalcularEstadoSinalizado(estado, ativa, true, 0);
        VERIFICAR_IGUAL(OPERACIONAL, estado);

        // Nao sobrescreve um estado definido por outra parte.
        ativa = false;
        estado = CalcularEstadoSinalizado(LACRE_ROMPIDO, ativa, false, 0);
        VERIFICAR_IGUAL(LACRE_ROMPIDO, estado);
        VERIFICAR(!ativa);
        estado = CalcularEstadoSinalizado(MANUTENCAO, ativa, true, 0);
        VERIFICAR_IGUAL(MANUTENCAO, estado);
    }
}

int main()
{
    TestarClassificacao();
    TestarEsperas();
    TestarSequenciaDeFalhas();
    TestarAttemptCount();
    TestarRejeicao();
    TestarInterpretarRetryAfter();
    TestarRetryAfter429();
    TestarPausaDescartaSoARejeitada();
    TestarVerificacaoNaPausa();
    TestarSemCicloRapidoComVerificacaoEnganosa();
    TestarOutrasRespostasNaoEncerramPausa();
    TestarSinalizacao();

    return teste::Resultado("test_politica_envio");
}
