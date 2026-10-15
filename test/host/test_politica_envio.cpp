// Testes de src/PoliticaEnvio.cpp: classificacao HTTP, esperas,
// Retry-After, destino da mensagem na fila, pausa e sinalizacao.

#include "teste.h"

#include <PoliticaEnvio.h>

namespace
{
    // Simula um envio e devolve a decisao, como o main.cpp faz.
    DecisaoEnvio Enviar(
        EstadoEnvio& estado,
        int codigo,
        uint32_t retryAfter = 0,
        bool realizada = true)
    {
        return DecidirAposEnvio(
            estado,
            ClassificarRespostaHTTP(codigo),
            codigo,
            retryAfter,
            realizada
        );
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
        VERIFICAR_IGUAL(60000, EsperaVerificacaoPausa(1));
        VERIFICAR_IGUAL(120000, EsperaVerificacaoPausa(2));
        VERIFICAR_IGUAL(300000, EsperaVerificacaoPausa(3));
        VERIFICAR_IGUAL(300000, EsperaVerificacaoPausa(200));
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
        VERIFICAR(Enviar(estado, 401).registrarTentativa);

        // Mensagem que sai da fila nao e regravada.
        IniciarEstadoEnvio(estado);
        VERIFICAR(!Enviar(estado, 409).registrarTentativa);
        VERIFICAR(!Enviar(estado, 400).registrarTentativa);
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

        VERIFICAR_IGUAL(FILA_DESCARTAR, Enviar(estado, 413).fila);
    }

    void TestarRetryAfter()
    {
        VERIFICAR_IGUAL(120, InterpretarRetryAfter("120"));
        VERIFICAR_IGUAL(30, InterpretarRetryAfter("  30 "));
        VERIFICAR_IGUAL(0, InterpretarRetryAfter("0"));
        VERIFICAR_IGUAL(0, InterpretarRetryAfter(""));
        VERIFICAR_IGUAL(0, InterpretarRetryAfter(nullptr));
        VERIFICAR_IGUAL(0, InterpretarRetryAfter("-5"));
        VERIFICAR_IGUAL(0, InterpretarRetryAfter("12abc"));
        VERIFICAR_IGUAL(0, InterpretarRetryAfter("Wed, 21 Oct 2026 07:28:00 GMT"));
        VERIFICAR_IGUAL(0xFFFFFFFFUL, InterpretarRetryAfter("99999999999999"));

        uint32_t segundos = 0;
        VERIFICAR(ExtrairRetryAfter("Retry-After: 45", segundos));
        VERIFICAR_IGUAL(45, segundos);
        VERIFICAR(ExtrairRetryAfter("retry-after:7", segundos));
        VERIFICAR_IGUAL(7, segundos);
        VERIFICAR(ExtrairRetryAfter("RETRY-AFTER : 9", segundos));
        VERIFICAR_IGUAL(9, segundos);
        VERIFICAR(!ExtrairRetryAfter("Content-Length: 12", segundos));
        VERIFICAR(!ExtrairRetryAfter("Retry-After-Extra: 3", segundos));
        VERIFICAR(!ExtrairRetryAfter("", segundos));

        EstadoEnvio estado;
        IniciarEstadoEnvio(estado);

        // 429 respeita o Retry-After do servidor.
        DecisaoEnvio decisao = Enviar(estado, 429, 45);
        VERIFICAR_IGUAL(FILA_MANTER, decisao.fila);
        VERIFICAR_IGUAL(45000, estado.intervaloMs);

        // Sem Retry-After: espera crescente.
        IniciarEstadoEnvio(estado);
        Enviar(estado, 429, 0);
        VERIFICAR_IGUAL(5000, estado.intervaloMs);

        // Teto de seguranca (inalterado nesta etapa) e sem estouro.
        Enviar(estado, 429, 0xFFFFFFFFUL);
        VERIFICAR_IGUAL(ESPERA_MAXIMA_RETRY_AFTER_MS, estado.intervaloMs);
        Enviar(estado, 429, 900);
        VERIFICAR_IGUAL(900000, estado.intervaloMs);
        Enviar(estado, 429, 901);
        VERIFICAR_IGUAL(900000, estado.intervaloMs);
    }

    void TestarPausa()
    {
        EstadoEnvio estado;
        IniciarEstadoEnvio(estado);

        // 401: mensagem mantida, pausa e primeira verificacao em 30 s.
        DecisaoEnvio decisao = Enviar(estado, 401);
        VERIFICAR_IGUAL(FILA_MANTER, decisao.fila);
        VERIFICAR_IGUAL(401, estado.codigoPausa);
        VERIFICAR_IGUAL(30000, estado.intervaloMs);

        // Verificacoes que falham espacam ate 5 min, sem apagar nada.
        DecidirAposVerificacao(estado, 401, 0);
        VERIFICAR_IGUAL(60000, estado.intervaloMs);
        DecidirAposVerificacao(estado, -3, 0); // rede: continua pausado
        VERIFICAR_IGUAL(401, estado.codigoPausa);
        VERIFICAR_IGUAL(120000, estado.intervaloMs);
        DecidirAposVerificacao(estado, 403, 0); // motivo atualizado
        VERIFICAR_IGUAL(403, estado.codigoPausa);
        VERIFICAR_IGUAL(300000, estado.intervaloMs);
        DecidirAposVerificacao(estado, 500, 0);
        VERIFICAR_IGUAL(300000, estado.intervaloMs);

        // Nada abaixo de 30 s durante a pausa (sem ciclo rapido).
        VERIFICAR(estado.intervaloMs >= 30000);

        // 429 na verificacao respeita o Retry-After.
        DecidirAposVerificacao(estado, 429, 50);
        VERIFICAR_IGUAL(50000, estado.intervaloMs);
        VERIFICAR(estado.codigoPausa != 0);

        // Outros 2xx nao bastam como prova de autorizacao.
        DecidirAposVerificacao(estado, 202, 0);
        VERIFICAR(estado.codigoPausa != 0);

        // 200 retoma o envio normal imediatamente.
        DecidirAposVerificacao(estado, 200, 0);
        VERIFICAR_IGUAL(0, estado.codigoPausa);
        VERIFICAR_IGUAL(0, estado.verificacoesPausa);
        VERIFICAR_IGUAL(2000, estado.intervaloMs);

        // Uma nova pausa recomeca a sequencia de verificacoes.
        Enviar(estado, 404);
        VERIFICAR_IGUAL(404, estado.codigoPausa);
        VERIFICAR_IGUAL(30000, estado.intervaloMs);
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
    TestarRetryAfter();
    TestarPausa();
    TestarSinalizacao();

    return teste::Resultado("test_politica_envio");
}
