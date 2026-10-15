
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
#include <Estado.h>
#include <PoliticaEnvio.h>

// ATENCAO: usar true somente durante o teste controlado.
// Depois, alterar para false.
const bool MODO_TESTE_TELEMETRIA = false;

const uint32_t INTERVALO_NOVA_TELEMETRIA = 10000UL;
    // MODO_TESTE_TELEMETRIA ? 10000UL : 30000UL;

// Nova tentativa de montar o LittleFS, sem formatar.
const uint32_t INTERVALO_REMONTAGEM_FILA = 30000UL;

uint32_t ultimoRegistroTelemetria = 0;
uint32_t ultimaTentativaEnvio = 0;
uint32_t ultimaTentativaFila = 0;

// Esperas, falhas seguidas e pausa por 401/403/404 (PoliticaEnvio).
EstadoEnvio estadoEnvio;

bool filaDisponivel = false;

// true quando estadoAtual foi alterado por AtualizarSinalizacao().
bool sinalizacaoAtiva = false;


// Reflete nos LEDs uma fila indisponivel ou um envio pausado.
void AtualizarSinalizacao()
{
    estadoAtual = CalcularEstadoSinalizado(
        estadoAtual,
        sinalizacaoAtiva,
        filaDisponivel,
        estadoEnvio.codigoPausa
    );
}


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

    IniciarEstadoEnvio(estadoEnvio);

    filaDisponivel = InicializarFilaOffline();
    ultimaTentativaFila = millis();


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
            !dadosTelemetria.tem_posicao_valida ? "Nao"
            : dadosTelemetria.gps_ok ? "Sim (atual)"
            : "Sim (ultima conhecida, gps_ok=false)"
        );

        Serial.print("Bateria reportada: ");
        Serial.println(dadosDispositivo.battery_percent);

        Serial.print("Fila offline: ");
        Serial.println(
            filaDisponivel
                ? "OK"
                : "INDISPONIVEL (LittleFS nao montou; nenhum dado apagado)"
        );

        Serial.print("Mensagens na fila: ");
        Serial.println(QuantidadeNaFilaOffline());

        if (estadoEnvio.codigoPausa != 0)
        {
            Serial.print("Envio PAUSADO (HTTP ");
            Serial.print(estadoEnvio.codigoPausa);
            Serial.println(
                "): conferir chave e cadastro. Mensagens preservadas."
            );
        }

        Serial.println("-----------------------");
    }

    // 0. Sem fila, tenta montar o LittleFS de novo (sem formatar).
    if (!filaDisponivel &&
        agora - ultimaTentativaFila >= INTERVALO_REMONTAGEM_FILA)
    {
        ultimaTentativaFila = agora;
        filaDisponivel = InicializarFilaOffline();
    }

    AtualizarSinalizacao();

    // 1. Criar e persistir uma nova telemetria.
    // Depois do primeiro fix, continua enviando: sem fix atual, vai a
    // ultima posicao valida com gps_ok=false. Antes do primeiro fix, nada.
    bool localizacaoDisponivel = dadosTelemetria.tem_posicao_valida;

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

        // Reavalia a idade do fix: se o GPS parou de mandar dados,
        // gps_ok passa a false em vez de repetir uma posicao velha.
        if (!MODO_TESTE_TELEMETRIA)
        {
            ObterDadosGPS();
        }

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
    bool envioLiberado =
        filaDisponivel &&
        WiFi.status() == WL_CONNECTED &&
        agora - ultimaTentativaEnvio >= estadoEnvio.intervaloMs;

    if (envioLiberado && estadoEnvio.codigoPausa != 0)
    {
        // Pausa por chave/cadastro: verifica a autorizacao sem reenviar
        // telemetria e sem apagar nada; so um 200 retoma os envios.
        ResultadoEnvio verificacao = VerificarAutorizacaoAPI();

        ultimaTentativaEnvio = millis();

        DecidirAposVerificacao(
            estadoEnvio,
            verificacao.codigo,
            verificacao.retry_after_s
        );

        if (estadoEnvio.codigoPausa == 0)
        {
            Serial.println("Autorizacao confirmada; envios retomados.");
        }
        else
        {
            Serial.print("Envio continua pausado. Nova verificacao em (s): ");
            Serial.println(estadoEnvio.intervaloMs / 1000UL);
        }
    }
    else if (envioLiberado)
    {
        String caminho;
        String payload;

        if (ObterProximaDaFilaOffline(caminho, payload))
        {
            Serial.println();
            Serial.print("Enviando mensagem da fila: ");
            Serial.println(caminho);

            ResultadoEnvio resultado = EnviarDadosTelemetria(payload);

            ultimaTentativaEnvio = millis();

            Serial.print("Resposta HTTP: ");
            Serial.println(resultado.codigo);

            DecisaoEnvio decisao = DecidirAposEnvio(
                estadoEnvio,
                resultado.acao,
                resultado.codigo,
                resultado.retry_after_s,
                resultado.requisicao_realizada
            );

            // Mesmo message_id e attempt_count + 1, uma vez por tentativa
            // realizada em que a mensagem continua na fila.
            if (decisao.registrarTentativa)
            {
                RegistrarTentativaNaFilaOffline(caminho, payload);
            }

            switch (decisao.fila)
            {
                case FILA_REMOVER:
                    if (!RemoverDaFilaOffline(caminho))
                    {
                        Serial.println(
                            "AVISO: API confirmou, mas nao "
                            "foi possivel remover o arquivo."
                        );
                    }
                    break;

                case FILA_DESCARTAR:
                    // Corpo invalido: reenviar nao resolve; libera a fila.
                    if (!DescartarDaFilaOffline(
                            caminho, payload, resultado.codigo))
                    {
                        Serial.println(
                            "AVISO: nao foi possivel descartar o arquivo."
                        );
                    }
                    break;

                case FILA_MANTER:
                    if (estadoEnvio.codigoPausa != 0)
                    {
                        Serial.print(
                            "Envio pausado; mensagens preservadas. "
                            "Verificacao da chave em (s): "
                        );
                    }
                    else
                    {
                        Serial.print(
                            "Mensagem mantida na fila. Proxima tentativa em (s): "
                        );
                    }

                    Serial.println(estadoEnvio.intervaloMs / 1000UL);
                    break;
            }
        }
    }


    delay(10);
}
