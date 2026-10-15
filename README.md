# FluxID Firmware

Firmware do lacre IoT **FluxID** para ESP32-C3. O dispositivo lê a posição GPS, monta telemetrias em JSON, guarda cada mensagem numa fila persistente na flash e envia para a API FluxID por HTTP, retirando da fila apenas o que a API confirmou.

> **Estado:** MVP em desenvolvimento. O envio funciona por Wi-Fi; o envio por GSM/GPRS (SIM800L) ainda está desativado. Bateria e estado do lacre ainda não são medidos. Veja [Limitações conhecidas](#limitações-conhecidas).

## Hardware

ESP32-C3 DevKitM-1 com GPS (UART1, GPIOs 20/21), modem SIM800L (UART0 remapeada, GPIOs 6/7) e LEDs verde e vermelho (GPIOs 0 e 1). A tabela completa, com o estado de cada ligação e as observações elétricas pendentes, está em **[docs/hardware/PINOUT.md](docs/hardware/PINOUT.md)**.

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
pio run -t upload                # gravar o firmware
pio device monitor               # monitor serial (115200)
```

### Primeira gravação: LittleFS

O firmware **nunca formata a flash sozinho**: uma falha de montagem não prova que a partição está vazia, e formatar poderia apagar a fila offline. Numa placa nova, grave uma vez a imagem do LittleFS:

```sh
mkdir data                       # o Git não guarda pastas vazias; o buildfs exige data/
pio run -t uploadfs              # grava um LittleFS vazio
```

> **Atenção:** o `uploadfs` substitui todo o LittleFS e **apaga a fila offline**. Use apenas em placa nova ou quando a perda das mensagens guardadas for aceitável.

Se a montagem falhar, o monitor serial mostra `ERRO: nao foi possivel montar o LittleFS. Nenhum dado foi apagado.`, o diagnóstico mostra `Fila offline: INDISPONIVEL`, os LEDs entram no padrão `ERRO` e o firmware tenta montar de novo a cada 30 s. Enquanto a fila estiver indisponível, nenhuma telemetria nova é gerada.

## Como funciona

```
GPS ──► dadosTelemetria ──┐
Dispositivo / GSM ────────┼──► MontarJsonTelemetria() ──► Fila offline (LittleFS /queue)
Mensagem (message_id) ────┘                                      │
                                                                 ▼
                                    Wi-Fi conectado? ──► POST /iot/telemetries
                                                                 │
  200/201/202/409 ──► remove · 400/413 ──► descarta · 401/403/404 ──► mantém e pausa · 429/5xx/sem resposta ──► mantém e reenvia
```

A cada volta do `loop()`:

1. Atualiza Wi-Fi, GPS, diagnóstico GSM (a cada 30 s) e LEDs.
2. **A cada 10 s**, depois do primeiro fix, gera um `message_id` novo (`MSG-<device_id>-NNNNNN`, com contador persistido no NVS) e grava o JSON na fila. Sem fix atual, vai a última posição válida com `gps_ok: false`. Antes do primeiro fix, nada é gerado.
3. Com Wi-Fi conectado, envia a mensagem mais antiga da fila (a cada 2 s enquanto não houver falhas):
   - `200`, `201`, `202` ou `409`: remove da fila.
   - `400` ou `413` (corpo inválido): registra o código e o conteúdo no serial e **descarta**, porque reenviar a mesma mensagem não resolve.
   - `401`, `403` ou `404` (chave ou cadastro): **mantém a mensagem e pausa o envio de telemetria**. O diagnóstico mostra `Envio PAUSADO (HTTP ...)` e os LEDs vão para `NAO_CADASTRADO` (401/404) ou `DESATIVADO` (403). Durante a pausa, o firmware **não reenvia telemetria**: verifica a chave com `GET /iot/commands/{device_id}`, rota documentada do próprio dispositivo que usa a mesma `X-API-Key` e só lista comandos pendentes, após 30 s, 60 s, 120 s e depois a cada 5 min. Um `200` retoma os envios na hora. Reiniciar a placa também encerra a pausa. As telemetrias novas continuam sendo guardadas.
   - `429`: mantém e espera os segundos do `Retry-After`. Sem o cabeçalho, ou com ele em formato de data, usa a espera crescente abaixo.
   - `5xx`, outros códigos ou sem resposta: mantém e espera 5 s, 15 s, 30 s e depois 60 s a cada falha seguida, ficando em 60 s. Um sucesso volta ao intervalo de 2 s.
   - Sempre que uma tentativa é **realizada** e a mensagem continua na fila, o arquivo é regravado com o **mesmo** `message_id` e `attempt_count + 1`. Se nada foi transmitido (Wi-Fi caiu, URL inválida), o contador não muda.

**Teto do `Retry-After`:** a espera é limitada a 15 min. As janelas de limite da API são de 1 minuto (120 mensagens por lacre, 60 chaves erradas por endereço), então o `Retry-After` esperado fica abaixo de 60 s e o teto só age contra valores anormais. Nesse caso, a tentativa sai antes do pedido e, no pior caso, recebe outro `429`. Remover ou mudar o teto é uma decisão pendente.

**Tempos da requisição HTTP:** a resposta tem até 10 s para a linha de status, 3 s para os demais cabeçalhos e 1 s por pausa na leitura do corpo. A conexão TCP usa o padrão do `WiFiClient` (3 s). Com URL de IP, uma tentativa sem resposta bloqueia o loop por cerca de 13 s no pior caso típico. Com URL de nome de host, o DNS pode somar até cerca de 15 s. Durante esse tempo, a UART do GPS guarda só cerca de 0,4 s de NMEA a 9600 baud (buffer de 256 bytes mais FIFO de 128), então frases são perdidas. Se o bloqueio passar de 5 s, a próxima telemetria pode sair com `gps_ok:false` até chegar uma frase nova, porque o fix é considerado velho.
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
| Estado | Verde | Vermelho | Quando o firmware usa |
| --- | --- | --- | --- |
| `OPERACIONAL` | Pisca 100 ms a cada 4 s | Apagado | Funcionamento normal |
| `NAO_CADASTRADO` | Aceso | Apagado | Envio pausado por `401` ou `404` |
| `DESATIVADO` | Apagado | Pisca 1 s / 1 s | Envio pausado por `403` |
| `ERRO` | Apagado | Código curto-curto-longo-curto | LittleFS não montou (fila indisponível) |
| `LACRE_ROMPIDO` | Apagado | Aceso | Padrão definido, mas **nada aciona este estado**: não há sensor de rompimento |
| `MANUTENCAO` | Alterna com o vermelho a cada 500 ms | Alterna com o verde | Padrão definido, mas **nada aciona este estado**: não há modo de manutenção |

Ao mudar de estado, os dois LEDs são apagados e os temporizadores reiniciados, então nenhum LED fica aceso de um padrão anterior. O estado começa em `OPERACIONAL` ([src/Estado.cpp](src/Estado.cpp)). O `main.cpp` aplica `ERRO`, `NAO_CADASTRADO` e `DESATIVADO` e volta a `OPERACIONAL` quando o problema some, sem sobrescrever estados definidos por outras partes.

### Modo de teste

Em [src/main.cpp](src/main.cpp), `MODO_TESTE_TELEMETRIA = true` substitui GPS, bateria e lacre por dados sintéticos, para testar a fila e a API sem fix de GPS. **Mantenha `false` fora de testes controlados.**

## Estrutura

```
src/
  main.cpp          setup/loop, criação e envio de telemetrias
  GPS.*             leitura e parsing do GPS (TinyGPSPlus)
  GSM.*             diagnóstico do SIM800L e cliente GPRS (TinyGSM)
  WiFiConn.cpp      conexão e reconexão Wi-Fi (cabeçalho: WifiConn.h)
  HTTP.*            requisições à API (telemetria e verificação da chave)
  PoliticaEnvio.*   regras de resposta HTTP, esperas, pausa e sinalização (sem Arduino)
  CabecalhosHTTP.h  leitura dos cabeçalhos com limite de tempo
  JSON.*            montagem do payload de telemetria
  Mensagem.*        geração do message_id persistido em NVS
  FilaOffline.*     fila persistente em LittleFS
  Dispositivo.*     dados do dispositivo (bateria, lacre)
  Estado.* LED.*    estado do dispositivo e padrões de LED
include/
  secrets.example.h modelo de configuração (copiar para secrets.h)
test/host/          testes no computador (MSVC): run_tests.ps1
docs/
  hardware/         pinout e observações elétricas
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
- **Divergência no Guia:** as seções 5 e 6 dizem "não insistir" para `401/403/404` e definem os estados `NAO_CADASTRADO`/`DESATIVADO`. A seção 7 e o exemplo de código mandam descartar. O firmware preserva as mensagens e pausa o envio, à espera da confirmação do backend.

## Limitações conhecidas

- **GPRS desativado:** o envio usa apenas Wi-Fi. O caminho por GSM está comentado em [src/HTTP.cpp](src/HTTP.cpp).
- **Bateria fixa:** o JSON envia `battery_percent: 100` como valor provisório, e o lacre ainda não é medido.
- **Estados de pausa e fila na RAM:** a pausa por `401/403/404` e a espera crescente são esquecidas ao reiniciar, e a primeira mensagem é enviada logo após o boot.
- **Sem fila, sem telemetria:** enquanto o LittleFS não monta, as telemetrias novas não são geradas nem guardadas.
- **Flash cheia:** quando não há espaço, a gravação falha (`ERRO: telemetria nao foi persistida.`) e a mensagem nova é perdida.
- **Envio bloqueante:** enquanto espera a resposta HTTP (ver *Tempos da requisição HTTP*), o loop para, inclusive LEDs e leitura do GPS.
- **Fila sem limite de tamanho.**
- **Sem HTTPS**, nem rotas de ativação, eventos, alertas ou comandos.

O andamento detalhado está em [docs/project/MVP_CHECKLIST.md](docs/project/MVP_CHECKLIST.md).
