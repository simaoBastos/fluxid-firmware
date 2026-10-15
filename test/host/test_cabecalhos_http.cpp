// Testes de src/CabecalhosHTTP.h com um leitor simulado no lugar do
// HttpClient: cabecalhos normais, Retry-After, dados em partes,
// servidor parado ou desconectado no meio dos cabecalhos e linhas longas.
//
// Simulam o comportamento do HttpClient; nao comprovam a compatibilidade
// com a API real.

#include "teste.h"

#include <CabecalhosHTTP.h>

#include <string>

namespace
{
    uint32_t relogio = 0;

    uint32_t Agora()
    {
        return relogio;
    }

    // Imita o HttpClient: entrega bytes conforme chegam e detecta o
    // fim dos cabecalhos ("\r\n\r\n"), como HttpClient::readHeader().
    struct LeitorSimulado
    {
        std::string dados;            // o que o servidor envia
        size_t entregues = 0;         // bytes disponiveis ate agora
        size_t lidos = 0;
        size_t bytesPorEspera = 1000; // quantos bytes chegam a cada espera
        int sequenciaFim = 0;
        int leiturasSemDados = 0;
        // true: o servidor fecha a conexao depois de enviar tudo.
        bool fechaAoTerminar = false;

        bool endOfHeadersReached() { return sequenciaFim == 4; }

        int available() { return static_cast<int>(entregues - lidos); }

        int readHeader()
        {
            if (lidos >= entregues)
            {
                leiturasSemDados++;
                return -1;
            }

            char c = dados[lidos++];
            bool esperado =
                (sequenciaFim % 2 == 0) ? (c == '\r') : (c == '\n');
            sequenciaFim = esperado ? sequenciaFim + 1 : 0;
            return c;
        }

        bool conectado()
        {
            return !(fechaAoTerminar && entregues >= dados.size());
        }

        void esperar()
        {
            relogio += 1;
            entregues += bytesPorEspera;

            if (entregues > dados.size())
                entregues = dados.size();
        }
    };

    void TestarCabecalhosNormais()
    {
        relogio = 0;
        LeitorSimulado leitor;
        leitor.dados =
            "Content-Type: application/json\r\n"
            "Retry-After: 30\r\n"
            "Content-Length: 2\r\n"
            "\r\n"
            "{}";

        ResultadoCabecalhos r = LerCabecalhosHTTP(leitor, Agora, 3000);

        VERIFICAR(r.completos);
        VERIFICAR_IGUAL(30, r.retry_after_s);
        VERIFICAR(!r.retry_after_invalido);
        // O corpo nao e consumido junto com os cabecalhos.
        VERIFICAR_IGUAL('{', leitor.dados[leitor.lidos]);
        VERIFICAR_IGUAL(0, leitor.leiturasSemDados);
    }

    void TestarSemRetryAfter()
    {
        relogio = 0;
        LeitorSimulado leitor;
        leitor.dados = "Content-Length: 0\r\nConnection: close\r\n\r\n";

        ResultadoCabecalhos r = LerCabecalhosHTTP(leitor, Agora, 3000);

        VERIFICAR(r.completos);
        VERIFICAR_IGUAL(0, r.retry_after_s);
        VERIFICAR(!r.retry_after_invalido);
    }

    void TestarDadosEmPartes()
    {
        // Link lento: um byte por espera. Com o headerAvailable() da
        // biblioteca, cada espera viraria um 0xFF dentro da linha.
        relogio = 0;
        LeitorSimulado leitor;
        leitor.bytesPorEspera = 1;
        leitor.dados = "retry-after: 12\r\nX-A: b\r\n\r\n";

        ResultadoCabecalhos r = LerCabecalhosHTTP(leitor, Agora, 3000);

        VERIFICAR(r.completos);
        VERIFICAR_IGUAL(12, r.retry_after_s);
        VERIFICAR_IGUAL(0, leitor.leiturasSemDados);
    }

    void TestarServidorParado()
    {
        // Os cabecalhos param no meio e a conexao continua aberta: deve
        // desistir no tempo limite, em vez de girar para sempre.
        relogio = 0;
        LeitorSimulado leitor;
        leitor.dados = "Retry-After: 5\r\nContent-Le";

        ResultadoCabecalhos r = LerCabecalhosHTTP(leitor, Agora, 3000);

        VERIFICAR(!r.completos);
        VERIFICAR(relogio >= 3000);
        VERIFICAR(relogio <= 3001);
        // O que chegou completo antes do timeout continua valido.
        VERIFICAR_IGUAL(5, r.retry_after_s);
    }

    void TestarConexaoEncerradaNoMeio()
    {
        // O servidor fecha a conexao antes do fim dos cabecalhos: sai na
        // hora, sem esperar o tempo limite.
        relogio = 0;
        LeitorSimulado leitor;
        leitor.fechaAoTerminar = true;
        leitor.dados = "Content-Type: application/json\r\nRetry-Af";

        ResultadoCabecalhos r = LerCabecalhosHTTP(leitor, Agora, 3000);

        VERIFICAR(!r.completos);
        VERIFICAR(relogio < 10);
        // Linha incompleta nao vira Retry-After.
        VERIFICAR_IGUAL(0, r.retry_after_s);
        VERIFICAR(!r.retry_after_invalido);
    }

    void TestarSoLinhaDeStatus()
    {
        // Resposta sem cabecalhos e conexao fechada (ex.: HTTP/1.0).
        relogio = 0;
        LeitorSimulado leitor;
        leitor.fechaAoTerminar = true;
        leitor.dados = "";

        ResultadoCabecalhos r = LerCabecalhosHTTP(leitor, Agora, 3000);

        VERIFICAR(!r.completos);
        VERIFICAR(relogio < 10);
    }

    void TestarDadosRestantesAposFechar()
    {
        // O servidor ja fechou, mas os bytes ainda estao no buffer:
        // devem ser lidos antes de desistir.
        relogio = 0;
        LeitorSimulado leitor;
        leitor.fechaAoTerminar = true;
        leitor.dados = "Retry-After: 20\r\n\r\nok";
        leitor.entregues = leitor.dados.size();

        ResultadoCabecalhos r = LerCabecalhosHTTP(leitor, Agora, 3000);

        VERIFICAR(r.completos);
        VERIFICAR_IGUAL(20, r.retry_after_s);
        VERIFICAR_IGUAL('o', leitor.dados[leitor.lidos]);
    }

    void TestarRetryAfterInvalido()
    {
        relogio = 0;
        LeitorSimulado leitor;
        leitor.dados = "Retry-After: Wed, 21 Oct 2026 07:28:00 GMT\r\n\r\n";

        ResultadoCabecalhos r = LerCabecalhosHTTP(leitor, Agora, 3000);

        VERIFICAR(r.completos);
        VERIFICAR_IGUAL(0, r.retry_after_s);
        VERIFICAR(r.retry_after_invalido);
    }

    void TestarRetryAfterLongoESaturado()
    {
        relogio = 0;
        LeitorSimulado leitor;
        leitor.dados = "Retry-After: 99999999999\r\n\r\n";

        ResultadoCabecalhos r = LerCabecalhosHTTP(leitor, Agora, 3000);

        VERIFICAR(r.completos);
        VERIFICAR_IGUAL(0xFFFFFFFFUL, r.retry_after_s);
        VERIFICAR(!r.retry_after_invalido);
    }

    void TestarRetryAfterEmLinhaTruncada()
    {
        // Linha longa demais que comeca com "Retry-After:" nao e
        // confiavel: os digitos podem ter sido cortados.
        relogio = 0;
        LeitorSimulado leitor;
        leitor.dados = "Retry-After: " + std::string(200, '9') + "\r\n\r\n";

        ResultadoCabecalhos r = LerCabecalhosHTTP(leitor, Agora, 3000);

        VERIFICAR(r.completos);
        VERIFICAR_IGUAL(0, r.retry_after_s);
        VERIFICAR(!r.retry_after_invalido);
    }

    void TestarLinhaLonga()
    {
        relogio = 0;
        LeitorSimulado leitor;
        leitor.dados =
            "Set-Cookie: " + std::string(500, 'x') + "\r\n"
            "Retry-After: 8\r\n\r\n";

        ResultadoCabecalhos r = LerCabecalhosHTTP(leitor, Agora, 3000);

        VERIFICAR(r.completos);
        VERIFICAR_IGUAL(8, r.retry_after_s);
    }

    void TestarChunkedPassaDireto()
    {
        // O leitor nao trata o chunked; so nao pode consumir o corpo.
        relogio = 0;
        LeitorSimulado leitor;
        leitor.dados =
            "Transfer-Encoding: chunked\r\n\r\n"
            "2\r\n{}\r\n0\r\n\r\n";

        ResultadoCabecalhos r = LerCabecalhosHTTP(leitor, Agora, 3000);

        VERIFICAR(r.completos);
        VERIFICAR_IGUAL('2', leitor.dados[leitor.lidos]);
    }

    void TestarRelogioVoltando()
    {
        // millis() reinicia apos ~49 dias; a subtracao sem sinal deve
        // continuar medindo o tempo corretamente.
        relogio = 0xFFFFFF00UL;
        LeitorSimulado leitor;
        leitor.dados = "X: 1\r\n";

        ResultadoCabecalhos r = LerCabecalhosHTTP(leitor, Agora, 3000);

        VERIFICAR(!r.completos);
        VERIFICAR_IGUAL(3000, relogio - 0xFFFFFF00UL);
    }
}

int main()
{
    TestarCabecalhosNormais();
    TestarSemRetryAfter();
    TestarDadosEmPartes();
    TestarServidorParado();
    TestarConexaoEncerradaNoMeio();
    TestarSoLinhaDeStatus();
    TestarDadosRestantesAposFechar();
    TestarRetryAfterInvalido();
    TestarRetryAfterLongoESaturado();
    TestarRetryAfterEmLinhaTruncada();
    TestarLinhaLonga();
    TestarChunkedPassaDireto();
    TestarRelogioVoltando();

    return teste::Resultado("test_cabecalhos_http");
}
