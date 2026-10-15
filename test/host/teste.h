#pragma once

// Mini-framework dos testes no computador (sem dependencias externas).

#include <cstdio>

namespace teste
{
    inline int& Falhas()
    {
        static int falhas = 0;
        return falhas;
    }

    inline int& Verificacoes()
    {
        static int verificacoes = 0;
        return verificacoes;
    }

    inline int Resultado(const char* suite)
    {
        std::printf(
            "%s: %d verificacoes, %d falhas\n",
            suite,
            Verificacoes(),
            Falhas()
        );

        return Falhas() == 0 ? 0 : 1;
    }
}

#define VERIFICAR(condicao)                                              \
    do                                                                   \
    {                                                                    \
        teste::Verificacoes()++;                                         \
        if (!(condicao))                                                 \
        {                                                                \
            teste::Falhas()++;                                           \
            std::printf("FALHA %s:%d: %s\n", __FILE__, __LINE__, #condicao); \
        }                                                                \
    } while (0)

#define VERIFICAR_IGUAL(esperado, obtido)                                \
    do                                                                   \
    {                                                                    \
        teste::Verificacoes()++;                                         \
        long long e_ = static_cast<long long>(esperado);                 \
        long long o_ = static_cast<long long>(obtido);                   \
        if (e_ != o_)                                                    \
        {                                                                \
            teste::Falhas()++;                                           \
            std::printf("FALHA %s:%d: %s esperado %lld, obtido %lld\n",  \
                        __FILE__, __LINE__, #obtido, e_, o_);            \
        }                                                                \
    } while (0)
