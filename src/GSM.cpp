
#define TINY_GSM_MODEM_SIM800

#include <Arduino.h>
#include <TinyGsmClient.h>
#include <GSM.h>
#include <secrets.h>

namespace
{
    HardwareSerial SerialSIM800(0);
    TinyGsm modem(SerialSIM800);
    TinyGsmClient clienteGSM(modem);

    constexpr uint32_t SIM800_BAUD_RATE = 9600;
    constexpr uint32_t INTERVALO_DIAGNOSTICO_MS = 30000;

    uint32_t ultimoDiagnostico = 0;
    bool modemInicializado = false;

    bool ConectarRedeMovel()
    {
        dadosGSM.estado_diagnostico = GSM_VERIFICANDO_REDE;

        if (!modem.waitForNetwork(1000L))
        {
            dadosGSM.rede_registrada = false;
            dadosGSM.dados_moveis_ativos = false;
            dadosGSM.estado_diagnostico = GSM_REDE_NAO_REGISTRADA;
            return false;
        }

        dadosGSM.rede_registrada = true;

        if (modem.isGprsConnected())
        {
            dadosGSM.dados_moveis_ativos = true;
            dadosGSM.estado_diagnostico = GSM_PRONTO_PARA_DADOS;
            return true;
        }

        dadosGSM.estado_diagnostico = GSM_VERIFICANDO_DADOS;

        if (strlen(FLUXID_GSM_APN) == 0)
        {
            dadosGSM.dados_moveis_ativos = false;
            dadosGSM.estado_diagnostico = GSM_DADOS_NAO_ATIVOS;
            return false;
        }

        bool conectado = modem.gprsConnect(
            FLUXID_GSM_APN,
            FLUXID_GSM_USER,
            FLUXID_GSM_PASSWORD
        );

        dadosGSM.dados_moveis_ativos = conectado;
        dadosGSM.estado_diagnostico =
            conectado ? GSM_PRONTO_PARA_DADOS : GSM_DADOS_NAO_ATIVOS;

        return conectado;
    }
}

DadosGSM dadosGSM;

void InicializarGSM(uint8_t rxPin, uint8_t txPin)
{
    dadosGSM.gsm_signal = -113;
    dadosGSM.modem_respondendo = false;
    dadosGSM.sim_pronto = false;
    dadosGSM.rede_registrada = false;
    dadosGSM.dados_moveis_ativos = false;
    dadosGSM.estado_diagnostico = GSM_TESTANDO_MODEM;

    SerialSIM800.begin(
        SIM800_BAUD_RATE,
        SERIAL_8N1,
        rxPin,
        txPin
    );

    modemInicializado = false;
    ultimoDiagnostico = 0;
}

void AtualizarGSM()
{
    if (millis() - ultimoDiagnostico < INTERVALO_DIAGNOSTICO_MS)
        return;

    ultimoDiagnostico = millis();
    dadosGSM.estado_diagnostico = GSM_TESTANDO_MODEM;

    if (!modem.testAT(1000L))
    {
        dadosGSM.modem_respondendo = false;
        dadosGSM.sim_pronto = false;
        dadosGSM.rede_registrada = false;
        dadosGSM.dados_moveis_ativos = false;
        dadosGSM.estado_diagnostico = GSM_MODEM_SEM_RESPOSTA;
        modemInicializado = false;
        return;
    }

    dadosGSM.modem_respondendo = true;

    if (!modemInicializado)
    {
        modemInicializado = modem.init();

        if (!modemInicializado)
        {
            dadosGSM.estado_diagnostico = GSM_MODEM_SEM_RESPOSTA;
            return;
        }
    }

    dadosGSM.estado_diagnostico = GSM_VERIFICANDO_SIM;
    dadosGSM.sim_pronto = (modem.getSimStatus() == SIM_READY);

    if (!dadosGSM.sim_pronto)
    {
        dadosGSM.estado_diagnostico = GSM_SIM_INDISPONIVEL;
        dadosGSM.dados_moveis_ativos = false;
        return;
    }

    if (ConectarRedeMovel())
    {
        int sinal = modem.getSignalQuality();

        if (sinal >= 0 && sinal <= 31)
        {
            dadosGSM.gsm_signal =
                static_cast<int8_t>(-113 + (2 * sinal));
        }
    }
}

void ObterDadosGSM()
{
    if (modemInicializado && modem.isGprsConnected())
    {
        int sinal = modem.getSignalQuality();

        if (sinal >= 0 && sinal <= 31)
        {
            dadosGSM.gsm_signal =
                static_cast<int8_t>(-113 + (2 * sinal));
        }
    }
}

bool GPRSConectado()
{
    return modemInicializado &&
           dadosGSM.dados_moveis_ativos &&
           modem.isGprsConnected();
}

Client* ObterClienteGSM()
{
    if (!GPRSConectado())
        return nullptr;

    return &clienteGSM;
}

const char* TextoEstadoGSM()
{
    switch (dadosGSM.estado_diagnostico)
    {
        case GSM_NAO_INICIADO:
            return "nao iniciado";

        case GSM_TESTANDO_MODEM:
            return "testando modem";

        case GSM_MODEM_SEM_RESPOSTA:
            return "modem sem resposta";

        case GSM_VERIFICANDO_SIM:
            return "verificando SIM";

        case GSM_SIM_INDISPONIVEL:
            return "SIM indisponivel";

        case GSM_VERIFICANDO_REDE:
            return "verificando rede";

        case GSM_REDE_NAO_REGISTRADA:
            return "rede nao registrada";

        case GSM_VERIFICANDO_DADOS:
            return "conectando GPRS";

        case GSM_DADOS_NAO_ATIVOS:
            return "GPRS indisponivel";

        case GSM_PRONTO_PARA_DADOS:
            return "GPRS conectado";
    }

    return "estado desconhecido";
}
