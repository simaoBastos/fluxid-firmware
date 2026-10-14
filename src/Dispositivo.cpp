#include <Dispositivo.h>
#include <secrets.h>

DadosDispositivo dadosDispositivo;

void InicializarDispositivo()
{
    dadosDispositivo.device_id = FLUXID_DEVICE_ID;

    // Ainda não existe medição real da bateria.
    dadosDispositivo.battery_percent = -1;

    // O estado físico do lacre ainda não é medido.
    dadosDispositivo.seal_status = "UNKNOWN";
}

void ObterDadosDispositivo()
{
    // Reservado para futuras leituras reais de hardware.
}