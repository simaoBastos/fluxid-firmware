# Contexto atual do FluxID firmware

Atualizado em 15/10/2026 (branch `fix/fila-contrato-api`, PR #1).

O projeto usa PlatformIO, Arduino e ESP32-C3 DevKitM-1 (Espressif32 7.1.3). Bibliotecas: TinyGPSPlus, ArduinoJson 7, TinyGSM e ArduinoHttpClient 0.6.2. A pinagem esta em [docs/hardware/PINOUT.md](../hardware/PINOUT.md).

## Modulos existentes

- **GPS:** UART1 (GPIO 20/21), parsing de localizacao, velocidade e satelites com TinyGPSPlus. O gatilho de leitura e `GPS.satellites.isUpdated()`. Depois do primeiro fix, a ultima posicao valida e preservada e enviada com `gps_ok:false` quando o fix se perde. Fix real informado pela equipe.
- **Wi-Fi:** transporte atual da telemetria, com reconexao a cada 10 s. Credenciais em `include/secrets.h` (ignorado pelo Git).
- **HTTP** (`HTTP.cpp`): `POST /iot/telemetries` com `X-API-Key`; timeout de resposta de 10 s; leitura de cabecalhos com limite de tempo. Durante uma pausa por 401/403/404, faz `GET /iot/commands/{device_id}` so para verificar a chave.
- **Politica de envio** (`PoliticaEnvio.cpp`): classificacao das respostas, esperas (5 s, 15 s, 30 s, 60 s), `Retry-After`, pausa e sinalizacao. Nao depende do Arduino e e testada no computador.
- **Fila offline** (`FilaOffline.cpp`): LittleFS em `/queue`, gravacao com `.tmp` e rename, ordem FIFO, `attempt_count + 1` por tentativa realizada. Nunca formata automaticamente.
- **Mensagem:** `message_id` no formato `MSG-<device_id>-NNNNNN`, com contador em NVS.
- **Estado e LEDs:** padroes nao bloqueantes para todos os estados; os dois LEDs sao apagados a cada troca de estado.
- **Dispositivo:** bateria e lacre sem leitura fisica; o JSON envia `battery_percent` provisorio igual a 100.
- **GSM:** diagnostico do SIM800L via TinyGSM na UART0 (GPIO 6/7). Nao foi testado em bancada e nao transporta telemetria.

## Testes

- Compilacao: `pio run -e esp32-c3-devkitm-1`.
- Computador: `test/host/run_tests.ps1` (MSVC), com politica de envio, cabecalhos HTTP e LEDs.
- Bancada: nenhum teste da branch atual. Ver [Questionario_Ciclo_01.md](../ai-records/ciclos/Questionario_Ciclo_01.md), parte B.

## Limitacoes relevantes

- Nao implementados: sensor de rompimento, rele/solenoide, medicao de bateria, deep sleep, eventos, alertas, comandos, HTTPS e HTTP por GSM.
- A requisicao HTTP bloqueia o loop (estimativa de cerca de 13 s sem resposta com URL de IP; nao medido).
- A fila nao tem limite de tamanho; a capacidade real da particao de 1,375 MB nao foi medida.
- 401/403/404: o firmware descarta so a mensagem rejeitada (secao 7 do Guia) e pausa as demais (secoes 5 e 6). Implementado por orientacao da revisao corretiva de 15/10; aguarda confirmacao no questionario (A3).
- A perda de frases NMEA durante bloqueios longos do HTTP ainda precisa ser medida na bancada.
