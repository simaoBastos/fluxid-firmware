# FluxID Firmware

Firmware do lacre IoT **FluxID** para ESP32-C3. O dispositivo lê a posição GPS, monta telemetrias em JSON, guarda cada mensagem numa fila persistente na flash e envia para a API FluxID por HTTP, retirando da fila apenas o que a API confirmou.

> **Estado:** MVP em desenvolvimento. O envio funciona por Wi-Fi; o envio por GSM/GPRS (SIM800L) ainda está desativado. Bateria e estado do lacre ainda não são medidos. Veja [Limitações conhecidas](#limitações-conhecidas).

## Hardware

| Componente | Ligação no ESP32-C3 DevKitM-1 |
| --- | --- |
| Módulo GPS (UART, 9600 baud) | GPIO 20 (RX), GPIO 21 (TX) |
| Modem SIM800L (UART, 9600 baud) | GPIO 6 (RX do ESP32), GPIO 7 (TX do ESP32) |
| LED verde | GPIO 0 |
| LED vermelho | GPIO 1 |

O monitor serial usa USB CDC a 115200 baud.

> Antes de ligar o SIM800L, confirme a alimentação dedicada (picos de cerca de 2 A), o GND comum, a antena, o chip SIM e o nível lógico da UART. Essa montagem ainda não foi validada na bancada.

## Requisitos

- [PlatformIO](https://platformio.org/) (extensão do VS Code ou CLI)
- Plataforma `espressif32` com framework Arduino
- Bibliotecas, instaladas automaticamente pelo `platformio.ini`: TinyGPSPlus, ArduinoJson 7, TinyGSM, ArduinoHttpClient

## Configuração

As credenciais ficam em `include/secrets.h`. Esse arquivo é ignorado pelo Git e **nunca deve ser versionado**.

```sh
cp include/secrets.example.h include/secrets.h
```

Depois, preencha os valores:

| Macro | Descrição |
| --- | --- |
| `FLUXID_WIFI_SSID` / `FLUXID_WIFI_PASSWORD` | Rede Wi-Fi |
| `FLUXID_API_BASE_URL` | Base da API, por exemplo `http://<IP_DA_API>:3000/api/v1` (só `http://` por enquanto) |
| `FLUXID_DEVICE_API_KEY` | Chave `X-API-Key` do dispositivo, fornecida pelo responsável pelo backend |
| `FLUXID_DEVICE_ID` | Identificador do dispositivo já cadastrado na API |
| `FLUXID_GSM_APN` / `_USER` / `_PASSWORD` | Dados da operadora para GPRS. Se o APN ficar vazio, o firmware não tenta conectar os dados móveis |

A API não cadastra dispositivos automaticamente: o cadastro e a chave precisam existir antes do primeiro envio.

## Compilar e gravar

```sh
pio run                          # compilar
pio run -t uploadfs              # gravar o sistema de arquivos LittleFS (necessário na primeira vez)
pio run -t upload                # gravar o firmware
pio device monitor               # monitor serial (115200)
```

O `uploadfs` é necessário numa placa nova porque o firmware monta o LittleFS **sem formatar automaticamente**. Se a partição não existir, a fila offline fica desativada e nenhuma telemetria é guardada.

## Como funciona

```
GPS ──► dadosTelemetria ──┐
Dispositivo / GSM ────────┼──► MontarJsonTelemetria() ──► Fila offline (LittleFS /queue)
Mensagem (message_id) ────┘                                      │
                                                                 ▼
                                    Wi-Fi conectado? ──► POST /iot/telemetries
                                                                 │
                         2xx ou 409 ──► remove da fila · outro resultado ──► mantém na fila
```

A cada volta do `loop()`:

1. Atualiza Wi-Fi, GPS, diagnóstico GSM (a cada 30 s) e LEDs.
2. **A cada 10 s**, com posição GPS disponível, gera um `message_id` novo (`MSG-<device_id>-NNNNNN`, com contador persistido no NVS) e grava o JSON na fila.
3. **A cada 2 s**, com Wi-Fi conectado, envia a mensagem mais antiga da fila. A mensagem só é removida depois de uma resposta `200`, `201`, `202` ou `409`.
4. A cada 3 s, imprime um diagnóstico no monitor serial: Wi-Fi, GSM, posição, bateria e tamanho da fila.

A fila grava cada mensagem primeiro num arquivo `.tmp` e depois renomeia para `.json`, para que uma queda de energia não deixe mensagens corrompidas. Temporários abandonados são removidos na inicialização.

### Exemplo de payload

```json
{
  "device_id": "<FLUXID_DEVICE_ID>",
  "message_id": "MSG-<FLUXID_DEVICE_ID>-000042",
  "latitude": -23.550520,
  "longitude": -46.633308,
  "gps_ok": true,
  "satellites": 9,
  "hdop": 0.9,
  "speed_kmh": 0.0,
  "attempt_count": 0,
  "battery_percent": 100
}
```

`seal_status` só é incluído quando o valor é `LOCKED`, `UNLOCKED` ou `BROKEN`. `gsm_signal` (em dBm) só é incluído quando o modem responde.

### LEDs

| Estado | Padrão |
| --- | --- |
| `NAO_CADASTRADO` | Verde aceso |
| `OPERACIONAL` | Verde pisca 100 ms a cada 4 s |
| `LACRE_ROMPIDO` | Vermelho aceso |
| `ERRO` | Verde aceso, vermelho repete o código curto-curto-longo-curto |

O estado atual está fixo em `OPERACIONAL` ([src/Estado.cpp](src/Estado.cpp)).

### Modo de teste

Em [src/main.cpp](src/main.cpp), `MODO_TESTE_TELEMETRIA = true` substitui GPS, bateria e lacre por dados sintéticos, para testar a fila e a API sem fix de GPS. **Mantenha `false` fora de testes controlados.**

## Estrutura

```
src/
  main.cpp          setup/loop, criação e envio de telemetrias
  GPS.*             leitura e parsing do GPS (TinyGPSPlus)
  GSM.*             diagnóstico do SIM800L e cliente GPRS (TinyGSM)
  WiFiConn.cpp      conexão e reconexão Wi-Fi (cabeçalho: WifiConn.h)
  HTTP.*            POST para a API e tratamento dos códigos de resposta
  JSON.*            montagem do payload de telemetria
  Mensagem.*        geração do message_id persistido em NVS
  FilaOffline.*     fila persistente em LittleFS
  Dispositivo.*     dados do dispositivo (bateria, lacre)
  Estado.* LED.*    estado do dispositivo e padrões de LED
include/
  secrets.example.h modelo de configuração (copiar para secrets.h)
docs/
  integrations/api/ contrato e referência da API FluxID
  project/          contexto do projeto e checklist do MVP
  development/      handoff e registro de desenvolvimento
  ai-records/       registros de uso de IA (entrega INTEGRA)
```

## Contrato da API

Resumo em [docs/integrations/api/API_CONTRACT.md](docs/integrations/api/API_CONTRACT.md). As fontes originais estão em [docs/integrations/api/reference/](docs/integrations/api/reference/).

- `POST /iot/telemetries` com `Content-Type: application/json` e `X-API-Key`.
- Campos obrigatórios: `message_id`, `device_id`, `latitude`, `longitude`, `battery_percent`.
- Reenvios mantêm o mesmo `message_id` e incrementam `attempt_count`.
- `200/201/202/409` confirmam a entrega · `400/401/403/404/413` não devem ser reenviados · `429` deve respeitar `Retry-After` · `500` ou falta de resposta mantêm a mensagem na fila.

## Limitações conhecidas

- **GPRS desativado:** o envio usa apenas Wi-Fi. O caminho por GSM está comentado em [src/HTTP.cpp](src/HTTP.cpp).
- **Bateria fixa:** o JSON envia `battery_percent: 100` como valor provisório, e o lacre ainda não é medido.
- **Erros permanentes travam a fila:** mensagens que recebem `4xx` continuam na fila e são reenviadas, bloqueando as seguintes.
- **Sem `Retry-After` nem backoff** para `429` e `500`.
- **`attempt_count` não aumenta** nos reenvios, porque o valor fica gravado no JSON da fila.
- **Sem telemetria depois de perder o fix:** fora do modo de teste, o firmware para de enviar quando o GPS perde o sinal, em vez de enviar a última posição com `gps_ok: false`.
- **Fila sem limite de tamanho.**
- **Sem HTTPS**, nem rotas de ativação, eventos, alertas ou comandos.

O andamento detalhado está em [docs/project/MVP_CHECKLIST.md](docs/project/MVP_CHECKLIST.md).
