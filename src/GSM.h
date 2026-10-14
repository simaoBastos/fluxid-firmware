
#pragma once

#include <Arduino.h>

enum EstadoDiagnosticoGSM : uint8_t
{
    GSM_NAO_INICIADO,
    GSM_TESTANDO_MODEM,
    GSM_MODEM_SEM_RESPOSTA,
    GSM_VERIFICANDO_SIM,
    GSM_SIM_INDISPONIVEL,
    GSM_VERIFICANDO_REDE,
    GSM_REDE_NAO_REGISTRADA,
    GSM_VERIFICANDO_DADOS,
    GSM_DADOS_NAO_ATIVOS,
    GSM_PRONTO_PARA_DADOS
};

struct DadosGSM
{
    int8_t gsm_signal;
    bool modem_respondendo;
    bool sim_pronto;
    bool rede_registrada;
    bool dados_moveis_ativos;
    EstadoDiagnosticoGSM estado_diagnostico;
};

extern DadosGSM dadosGSM;

void InicializarGSM(uint8_t rxPin, uint8_t txPin);
void AtualizarGSM();
void ObterDadosGSM();

bool GPRSConectado();
Client* ObterClienteGSM();

const char* TextoEstadoGSM();
