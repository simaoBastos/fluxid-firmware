#include <Arduino.h>
#include <LED.h>
#include <Estado.h>

uint8_t ledVerde;
uint8_t ledVermelho;
bool ledVerdeLigado = false;
bool ledVermelhoLigado = false;
uint32_t ultimoEnvioLED = 0;

enum SimbolosCodigo
{
    CURTO,
    LONGO
};

SimbolosCodigo erro1[] = {CURTO, CURTO, LONGO, CURTO};
uint8_t indiceAtual = 0;

const uint32_t TEMPO_CURTO = 150;
const uint32_t TEMPO_LONGO = 500;
const uint32_t INTERVALO_SIMBOLOS = TEMPO_CURTO;
const uint32_t INTERVALO_CODIGO = 1000;

void PiscarLED(uint32_t tempoLigado, uint32_t tempoDesligado)
{
uint32_t agora = millis();

    if (ledVerdeLigado)
    {
        if (agora - ultimoEnvioLED >= tempoLigado)
        {
            digitalWrite(ledVerde, LOW);

            ledVerdeLigado = false;
            
        
            ultimoEnvioLED = agora;
        }
        
    }
    else
    {
        if (agora - ultimoEnvioLED >= tempoDesligado)
        {
            digitalWrite(ledVerde, HIGH);
            
            ledVerdeLigado = true;
        
            ultimoEnvioLED = agora;
        }
    }
}

void ReproduzirCodigoErro()
{
uint32_t agora = millis();


    if (indiceAtual >= sizeof(erro1)/sizeof(erro1[0]))
    {
        if (agora-ultimoEnvioLED >= INTERVALO_CODIGO) indiceAtual = 0;
        return;
    }

    if (!ledVermelhoLigado)
    {
        if (agora-ultimoEnvioLED >= INTERVALO_SIMBOLOS)
        {
            ledVermelhoLigado = true;        
            ultimoEnvioLED = agora;
            return;
        }
    }

    switch (erro1[indiceAtual])
    {
    case CURTO:
        if (ledVermelhoLigado && agora-ultimoEnvioLED>=TEMPO_CURTO)
        {
            ledVermelhoLigado = false;
            indiceAtual++;
            ultimoEnvioLED = agora;
        }
            break;

    case LONGO:
        if (ledVermelhoLigado && agora-ultimoEnvioLED>=TEMPO_LONGO)
        {
            ledVermelhoLigado = false;
            indiceAtual++;
            ultimoEnvioLED = agora;
        }
        break;

    default:
        break;
    }
digitalWrite(ledVermelho, ledVermelhoLigado);
}

void InicializarLEDs(uint8_t pinoVerde, uint8_t pinoVermelho)
{
    ledVerde = pinoVerde;
    ledVermelho = pinoVermelho;
    pinMode(pinoVerde, OUTPUT);
    pinMode(pinoVermelho, OUTPUT);
    digitalWrite(pinoVerde, LOW);
    digitalWrite(pinoVermelho, LOW);
}

void AtualizarLEDs()
{
    switch (estadoAtual)
    {
    case NAO_CADASTRADO:
        digitalWrite(ledVerde, HIGH);
        digitalWrite(ledVermelho, LOW);
        break;

    case OPERACIONAL:
    {
        PiscarLED(100, 4000);
        
        break;
    }

    case LACRE_ROMPIDO:
        digitalWrite(ledVerde, LOW);
        digitalWrite(ledVermelho, HIGH);
        break;

    case ERRO:
        digitalWrite(ledVerde, HIGH);
        ReproduzirCodigoErro();
        break;
    
    default:
        break;
    }
}