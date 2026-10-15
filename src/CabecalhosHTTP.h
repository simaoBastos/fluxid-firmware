#pragma once

// Leitura dos cabecalhos da resposta HTTP com limite de tempo.
//
// HttpClient::headerAvailable() (ArduinoHttpClient 0.6.2) nao tem
// timeout: sem bytes disponiveis, readHeader() devolve -1, que entra
// na linha como 0xFF, e o laco gira ate os cabecalhos terminarem.
// Num link lento isso corrompe o Retry-After ou trava o loop.
//
// Os bytes sao lidos com HttpClient::readHeader(), que continua
// reconhecendo Content-Length e Transfer-Encoding: chunked. Por isso o
// corpo pode ser lido depois com responseBody(), como antes.
//
// Leitor precisa de: endOfHeadersReached(), available(), readHeader(),
// conectado() e esperar() (pausa curta enquanto nao ha dados).
// Relogio e um chamavel que devolve milissegundos (millis()).

#include <stddef.h>
#include <stdint.h>

#include <PoliticaEnvio.h>

struct ResultadoCabecalhos
{
    bool completos;            // false: tempo esgotado ou conexao encerrada antes do fim
    uint32_t retry_after_s;    // Retry-After valido (0 se ausente)
    bool retry_after_invalido; // Retry-After presente, mas ilegivel
};

template <typename Leitor, typename Relogio>
ResultadoCabecalhos LerCabecalhosHTTP(
    Leitor& leitor,
    Relogio agora,
    uint32_t timeoutMs)
{
    ResultadoCabecalhos resultado;
    resultado.completos = false;
    resultado.retry_after_s = 0;
    resultado.retry_after_invalido = false;

    // Linhas maiores sao truncadas; Retry-After cabe com folga.
    char linha[96];
    size_t tamanho = 0;
    bool truncada = false;

    uint32_t inicio = agora();

    while (!leitor.endOfHeadersReached())
    {
        if (agora() - inicio >= timeoutMs)
            return resultado;

        if (leitor.available() <= 0)
        {
            // Servidor fechou a conexao no meio dos cabecalhos.
            if (!leitor.conectado())
                return resultado;

            leitor.esperar();
            continue;
        }

        int c = leitor.readHeader();

        if (c < 0 || c == '\r')
            continue;

        if (c == '\n')
        {
            linha[tamanho] = '\0';

            uint32_t segundos = 0;
            bool valido = false;

            // Uma linha truncada nao e confiavel para o Retry-After.
            if (!truncada && ExtrairRetryAfter(linha, segundos, valido))
            {
                resultado.retry_after_s = valido ? segundos : 0;
                resultado.retry_after_invalido = !valido;
            }

            tamanho = 0;
            truncada = false;
            continue;
        }

        if (tamanho < sizeof(linha) - 1)
            linha[tamanho++] = static_cast<char>(c);
        else
            truncada = true;
    }

    resultado.completos = true;
    return resultado;
}
