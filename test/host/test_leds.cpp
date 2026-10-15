// Testes de src/LED.cpp com Arduino.h simulado (test/host/mocks).
// Verifica o padrao de cada estado e as transicoes entre estados,
// em especial que nenhum LED fica aceso de um padrao anterior.

#include "teste.h"

#include <Arduino.h>
#include <Estado.h>
#include <LED.h>

namespace
{
    const uint8_t VERDE = 0;
    const uint8_t VERMELHO = 1;

    int Verde() { return mock::Pinos()[VERDE]; }
    int Vermelho() { return mock::Pinos()[VERMELHO]; }

    // Avanca o tempo em passos de 10 ms, como o loop() (delay(10)).
    void Rodar(uint32_t ms)
    {
        for (uint32_t t = 0; t < ms; t += 10)
        {
            mock::Relogio() += 10;
            AtualizarLEDs();
        }
    }

    // Conta quanto tempo cada LED ficou aceso numa janela.
    struct Amostra
    {
        uint32_t verdeAceso = 0;
        uint32_t vermelhoAceso = 0;
        int trocasVerde = 0;
        int trocasVermelho = 0;
    };

    Amostra Amostrar(uint32_t ms)
    {
        Amostra a;
        int verdeAntes = Verde();
        int vermelhoAntes = Vermelho();

        for (uint32_t t = 0; t < ms; t += 10)
        {
            mock::Relogio() += 10;
            AtualizarLEDs();

            if (Verde()) a.verdeAceso += 10;
            if (Vermelho()) a.vermelhoAceso += 10;
            if (Verde() != verdeAntes) a.trocasVerde++;
            if (Vermelho() != vermelhoAntes) a.trocasVermelho++;

            verdeAntes = Verde();
            vermelhoAntes = Vermelho();
        }

        return a;
    }

    void Entrar(EstadoDispositivo estado)
    {
        estadoAtual = estado;
        Rodar(10);
    }

    // Avanca ate o vermelho acender durante o codigo de erro.
    bool EsperarVermelhoAceso(uint32_t limiteMs)
    {
        for (uint32_t t = 0; t < limiteMs; t += 10)
        {
            if (Vermelho()) return true;
            Rodar(10);
        }

        return Vermelho() == HIGH;
    }

    void TestarErroParaOperacional()
    {
        InicializarLEDs(VERDE, VERMELHO);
        mock::Relogio() = 1000;

        Entrar(ERRO);
        VERIFICAR(EsperarVermelhoAceso(3000));

        // Sai de ERRO com o vermelho aceso: deve apagar na hora.
        Entrar(OPERACIONAL);
        VERIFICAR_IGUAL(LOW, Vermelho());

        Amostra a = Amostrar(8000);
        VERIFICAR_IGUAL(0, a.vermelhoAceso);
        // Verde pisca 100 ms a cada ~4 s: 2 piscadas em 8 s.
        VERIFICAR(a.verdeAceso >= 100);
        VERIFICAR(a.verdeAceso <= 300);
    }

    void TestarOperacional()
    {
        InicializarLEDs(VERDE, VERMELHO);
        mock::Relogio() = 5000;
        Entrar(OPERACIONAL);

        Amostra a = Amostrar(12000);
        VERIFICAR_IGUAL(0, a.vermelhoAceso);
        VERIFICAR(a.trocasVerde >= 4); // pisca, nao fica fixo
        VERIFICAR(a.verdeAceso < 1000);
    }

    void TestarNaoCadastrado()
    {
        InicializarLEDs(VERDE, VERMELHO);
        mock::Relogio() = 0;
        Entrar(NAO_CADASTRADO);

        Amostra a = Amostrar(5000);
        VERIFICAR_IGUAL(5000, a.verdeAceso);   // verde fixo
        VERIFICAR_IGUAL(0, a.vermelhoAceso);
    }

    void TestarErroInequivoco()
    {
        InicializarLEDs(VERDE, VERMELHO);
        mock::Relogio() = 0;
        Entrar(ERRO);

        Amostra a = Amostrar(6000);
        // ERRO nao usa o verde, para nao confundir com NAO_CADASTRADO.
        VERIFICAR_IGUAL(0, a.verdeAceso);
        // Vermelho repete o codigo: varias trocas, nao fica fixo.
        VERIFICAR(a.trocasVermelho >= 8);
        VERIFICAR(a.vermelhoAceso > 0);
        VERIFICAR(a.vermelhoAceso < 6000);
    }

    void TestarDesativado()
    {
        InicializarLEDs(VERDE, VERMELHO);
        mock::Relogio() = 0;
        Entrar(DESATIVADO);

        Amostra a = Amostrar(10000);
        VERIFICAR_IGUAL(0, a.verdeAceso);
        // Vermelho pisca 1 s aceso / 1 s apagado.
        VERIFICAR(a.vermelhoAceso >= 4000);
        VERIFICAR(a.vermelhoAceso <= 6000);
        VERIFICAR(a.trocasVermelho >= 8);
    }

    void TestarManutencao()
    {
        InicializarLEDs(VERDE, VERMELHO);
        mock::Relogio() = 0;
        Entrar(MANUTENCAO);

        Amostra a = Amostrar(10000);
        // Alterna verde e vermelho a cada 500 ms.
        VERIFICAR(a.verdeAceso >= 4000 && a.verdeAceso <= 6000);
        VERIFICAR(a.vermelhoAceso >= 4000 && a.vermelhoAceso <= 6000);
        VERIFICAR(a.trocasVerde >= 18);

        // Nunca os dois acesos ao mesmo tempo.
        bool ambos = false;
        for (int i = 0; i < 500; i++)
        {
            Rodar(10);
            if (Verde() && Vermelho()) ambos = true;
        }
        VERIFICAR(!ambos);
    }

    void TestarLacreRompido()
    {
        InicializarLEDs(VERDE, VERMELHO);
        mock::Relogio() = 0;
        Entrar(LACRE_ROMPIDO);

        Amostra a = Amostrar(3000);
        VERIFICAR_IGUAL(0, a.verdeAceso);
        VERIFICAR_IGUAL(3000, a.vermelhoAceso);
    }

    void TestarTodasAsTransicoes()
    {
        const EstadoDispositivo estados[] = {
            NAO_CADASTRADO, OPERACIONAL, LACRE_ROMPIDO,
            ERRO, MANUTENCAO, DESATIVADO
        };

        // De cada estado para OPERACIONAL: o vermelho apaga na entrada
        // e o verde comeca apagado (o padrao de OPERACIONAL so acende
        // depois do intervalo).
        for (EstadoDispositivo origem : estados)
        {
            InicializarLEDs(VERDE, VERMELHO);
            mock::Relogio() = 100000;
            Entrar(origem);
            Rodar(1730); // ponto qualquer do padrao de origem

            Entrar(OPERACIONAL);
            VERIFICAR_IGUAL(LOW, Vermelho());
            VERIFICAR_IGUAL(LOW, Verde());
        }

        // De cada estado para cada estado: nenhum LED "herdado".
        for (EstadoDispositivo origem : estados)
        {
            for (EstadoDispositivo destino : estados)
            {
                InicializarLEDs(VERDE, VERMELHO);
                mock::Relogio() = 200000;
                Entrar(origem);
                Rodar(2370);
                Entrar(destino);

                if (destino == LACRE_ROMPIDO)
                {
                    VERIFICAR_IGUAL(LOW, Verde());
                    VERIFICAR_IGUAL(HIGH, Vermelho());
                }
                else if (destino == NAO_CADASTRADO)
                {
                    VERIFICAR_IGUAL(HIGH, Verde());
                    VERIFICAR_IGUAL(LOW, Vermelho());
                }
                else if (destino == MANUTENCAO)
                {
                    // Alternado: exatamente um aceso, nunca os dois.
                    VERIFICAR(Verde() != Vermelho());
                }
                else
                {
                    // Padroes piscantes comecam com os dois apagados.
                    VERIFICAR_IGUAL(LOW, Verde());
                    VERIFICAR_IGUAL(LOW, Vermelho());
                }
            }
        }
    }

    void TestarSemBloqueio()
    {
        // AtualizarLEDs() nao pode esperar: o relogio so anda pelo teste.
        InicializarLEDs(VERDE, VERMELHO);
        mock::Relogio() = 0;
        estadoAtual = ERRO;

        for (int i = 0; i < 1000; i++)
            AtualizarLEDs();

        VERIFICAR_IGUAL(0, mock::Relogio());
    }
}

int main()
{
    TestarErroParaOperacional();
    TestarOperacional();
    TestarNaoCadastrado();
    TestarErroInequivoco();
    TestarDesativado();
    TestarManutencao();
    TestarLacreRompido();
    TestarTodasAsTransicoes();
    TestarSemBloqueio();

    return teste::Resultado("test_leds");
}
