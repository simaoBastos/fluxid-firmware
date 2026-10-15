#pragma once

// Leitura dos cabecalhos da resposta HTTP com limite de tempo.
//
// HttpClient::headerAvailable() (ArduinoHttpClient 0.6.2) nao tem
// timeout: sem bytes disponiveis, readHeader() devolve -1, que entra
// na linha como 0xFF, e o laco gira ate os cabecalhos terminarem.
// Num link lento isso corrompe o Retry-After ou trava o loop.
//
// Leitor precisa de: endOfHeadersReached(), available(), readHeader()
// e esperar() (pausa curta enquanto nao ha dados).
// Relogio e um chamavel que devolve milissegundos (millis()).

#include <stddef.h>
#include <stdint.h>

#include <PoliticaEnvio.h>

struct ResultadoCabecalhos
{
    bool completos;          // false se o tempo acabou antes do fim
    uint32_t retry_after_s;  // 0 se ausente ou invalido
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

    // Linhas maiores sao truncadas; Retry-After cabe com folga.
    char linha[96];
    size_t tamanho = 0;

    uint32_t inicio = agora();

    while (!leitor.endOfHeadersReached())
    {
        if (agora() - inicio >= timeoutMs)
            return resultado;

        if (leitor.available() <= 0)
        {
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

            if (ExtrairRetryAfter(linha, segundos))
                resultado.retry_after_s = segundos;

            tamanho = 0;
            continue;
        }

        if (tamanho < sizeof(linha) - 1)
            linha[tamanho++] = static_cast<char>(c);
    }

    resultado.completos = true;
    return resultado;
}
