
#include <Arduino.h>
#include <WiFi.h>

#include <GPS.h>
#include <GSM.h>
#include <LED.h>
#include <HTTP.h>
#include <JSON.h>
#include <WiFiConn.h>
#include <Dispositivo.h>
#include <Mensagem.h>
#include <FilaOffline.h>

// ATENCAO: usar true somente durante o teste controlado.
// Depois, alterar para false.
const bool MODO_TESTE_TELEMETRIA = false;

const uint32_t INTERVALO_NOVA_TELEMETRIA = 10000UL;
    // MODO_TESTE_TELEMETRIA ? 10000UL : 30000UL;

const uint32_t INTERVALO_TENTATIVA_ENVIO = 2000UL;

uint32_t ultimoRegistroTelemetria = 0;
uint32_t ultimaTentativaEnvio = 0;

bool filaDisponivel = false;


void PrepararDadosDeTeste()
{
    // Dados sinteticos: nao representam sensores reais.
    dadosTelemetria.latitude = 0.0;
    dadosTelemetria.longitude = 0.0;
    dadosTelemetria.speed_kmh = 0.0;
    dadosTelemetria.satellites = 0;
    dadosTelemetria.hdop = 0.0;

    dadosTelemetria.gps_ok = true;
    dadosTelemetria.tem_posicao_valida = true;

    dadosDispositivo.battery_percent = 75;
    dadosDispositivo.seal_status = "LOCKED";
}

void setup()
{
    Serial.begin(115200);

    delay(3000); // Aguarda 3 segundos para abrir o Monitor Serial

    InicializarWiFi();
    InicializarLEDs(0, 1);
    InicializarGPS(20, 21);
    InicializarGSM(6, 7);
    InicializarDispositivo();
    InicializarMensagem();

    filaDisponivel = InicializarFilaOffline();
    

    Serial.print("Resultado da inicializacao da fila: ");
    Serial.println(filaDisponivel ? "SUCESSO" : "FALHA");

    // Serial.println("Limpando fila de teste..."); // Código para limpeza da fila offline
    // LimparFilaOffline();

    Serial.println();
    Serial.println("FluxID: inicializacao concluida.");

    if (MODO_TESTE_TELEMETRIA)
    {
        Serial.println(
            "ATENCAO: modo de teste ativo; "
            "bateria, lacre e coordenadas sao sinteticos."
        );
    }
}

void loop()
{
    VerificarConexaoWiFi();
    LerGPS();
    AtualizarGSM();
    AtualizarLEDs();

    
if (DadosGPSAtualizados())
{
    ObterDadosGPS();

    Serial.println();
    Serial.println("----- DIAGNOSTICO GPS -----");

    Serial.print("Satelites: ");
    Serial.println(dadosTelemetria.satellites);

    Serial.print("HDOP: ");
    Serial.println(dadosTelemetria.hdop);

    Serial.print("Fix GPS atual: ");
    Serial.println(dadosTelemetria.gps_ok ? "Valido" : "Sem fix");

    if (dadosTelemetria.gps_ok)
    {
        Serial.print("Latitude: ");
        Serial.println(dadosTelemetria.latitude, 6);

        Serial.print("Longitude: ");
        Serial.println(dadosTelemetria.longitude, 6);

        Serial.print("Velocidade (km/h): ");
        Serial.println(dadosTelemetria.speed_kmh);

        ObterDadosGSM();
        ObterDadosDispositivo();

        // NovaMensagem();

        Serial.println("JSON preparado:");
        Serial.println(MontarJsonTelemetria());
    }
    else
    {
        ImprimirLocalizacaoInvalida();
    }

    Serial.println("---------------------------");
}


    // ObterDadosGSM();
    ObterDadosDispositivo();

    if (MODO_TESTE_TELEMETRIA)
    {
        PrepararDadosDeTeste();
    }

    static uint32_t ultimoDiagnostico = 0;
    uint32_t agora = millis();

    if (agora - ultimoDiagnostico >= 3000UL)
    {
        ultimoDiagnostico = agora;

        Serial.println();
        Serial.println("----- DIAGNOSTICO -----");

        Serial.print("Wi-Fi: ");
        Serial.println(
            WiFi.status() == WL_CONNECTED
                ? "Conectado"
                : "Desconectado"
        );

        Serial.print("GSM: ");
        Serial.println(TextoEstadoGSM());

        Serial.print("Posicao utilizavel: ");
        Serial.println(
            MODO_TESTE_TELEMETRIA
            ? (dadosTelemetria.tem_posicao_valida ? "Sim" : "Nao")
            : (dadosTelemetria.gps_ok ? "Sim" : "Nao")
        );

        Serial.print("Bateria reportada: ");
        Serial.println(dadosDispositivo.battery_percent);

        Serial.print("Mensagens na fila: ");
        Serial.println(QuantidadeNaFilaOffline());

        Serial.println("-----------------------");
    }

    
    // 1. Criar e persistir uma nova telemetria.
    bool localizacaoDisponivel =
        MODO_TESTE_TELEMETRIA
            ? dadosTelemetria.tem_posicao_valida
            : dadosTelemetria.gps_ok;

    bool bateriaValida = true;
        // (dadosDispositivo.battery_percent >= 0 &&
        //  dadosDispositivo.battery_percent <= 100) ||
        // MODO_TESTE_TELEMETRIA;

    if (filaDisponivel &&
        localizacaoDisponivel &&
        bateriaValida &&
        agora - ultimoRegistroTelemetria >=
            INTERVALO_NOVA_TELEMETRIA)
    {
        // Evita tentativas contínuas se houver falha.
        ultimoRegistroTelemetria = agora;

        NovaMensagem();

        String payload = MontarJsonTelemetria();

        if (!payload.isEmpty())
        {
            Serial.println();
            Serial.println("Nova telemetria:");
            Serial.println(payload);

            if (AdicionarNaFilaOffline(payload))
            {
                Serial.println(
                    "Telemetria persistida com sucesso."
                );
            }
            else
            {
                Serial.println(
                    "ERRO: telemetria nao foi persistida."
                );
            }
        }
        else
        {
            Serial.println(
                "ERRO: JSON de telemetria vazio."
            );
        }
    }


    
    // 2. Tentar enviar a mensagem mais antiga.
    // Se nao houver Wi-Fi, ela permanece na fila.
    if (
        filaDisponivel &&
        WiFi.status() == WL_CONNECTED &&
        agora - ultimaTentativaEnvio >=
            INTERVALO_TENTATIVA_ENVIO
    )
    {
        String caminho;
        String payload;

        if (ObterProximaDaFilaOffline(caminho, payload))
        {
            ultimaTentativaEnvio = millis();

            Serial.println();
            Serial.print("Enviando mensagem da fila: ");
            Serial.println(caminho);

            int codigo = EnviarDadosTelemetria(payload);

            Serial.print("Resposta HTTP: ");
            Serial.println(codigo);

            if (
                codigo == 200 ||
                codigo == 201 ||
                codigo == 202 ||
                codigo == 409
            )
            {
                if (!RemoverDaFilaOffline(caminho))
                {
                    Serial.println(
                        "AVISO: API confirmou, mas nao "
                        "foi possivel remover o arquivo."
                    );
                }
            }
            else
            {
                Serial.println(
                    "Mensagem mantida na fila para nova tentativa."
                );
            }
        }
    }


    delay(10);
}
