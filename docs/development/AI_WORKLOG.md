# Registro tecnico de desenvolvimento assistido por IA

## 12/10/2026 - auditoria, configuracao local e diagnostico SIM800L

### Decisoes

- Foram lidos quatro documentos originais em `docs/integrations/api/reference/`; nenhum foi modificado.
- Foram preservados os tres documentos oficiais em `docs/ai-records/`.
- Foi escolhido AT direto em vez de TinyGSM. A etapa precisa somente de consultas AT com timeout; adicionar TinyGSM antes do teste fisico aumentaria dependencias sem validar UART, alimentacao ou rede.
- GPIO 6 (RX do ESP32) e GPIO 7 (TX do ESP32) foram mantidos conforme o planejamento. A Espressif documenta ambos como expostos na DevKitM-1; a validade eletrica da montagem continua pendente.
- `AT`, `AT+CPIN?`, `AT+CREG?`, `AT+CSQ`, `AT+CGATT?` e `AT+CGACT?` sao consultados em sequencia. A implementacao nao envia PIN, APN, chave ou HTTP.

### Auditoria relevante

- Credenciais estavam em fonte versionada e foram substituidas por macros de `secrets.h`, arquivo ignorado. Nenhum valor real foi copiado para documentos ou exemplos.
- O envio de telemetria ja estava desabilitado. Ele permanece desabilitado.
- Bateria, lacre e sinal GSM nao sao ainda leituras fisicas; funcoes correspondentes permanecem pendentes.
- Retentativa HTTP, fila persistente, leitura de respostas e conformidade completa do payload foram identificadas como proximas etapas, sem correcao nesta entrega.

### Compilacao e testes

- Compilacao inicial: sucesso com `pio run -e esp32-c3-devkitm-1`.
- Compilacao posterior: sucesso com `pio run -e esp32-c3-devkitm-1 --silent`.
- Durante a primeira compilacao posterior, o compilador apontou uma chave excedente e, em seguida, uma chave ausente em `GSM.cpp`. Ambas foram corrigidas antes da compilacao final bem-sucedida.
- Nenhum teste de modem, SIM, rede celular, dados moveis, GPS, LEDs ou API real foi executado nesta maquina.

## 14 e 15/10/2026 - Ciclo 01: fila offline, contrato HTTP, LEDs e pinout (Claude Code)

### Decisoes

- **Correcao do registro de 12/10:** o `GSM.cpp` atual usa TinyGSM (`TinyGsmClient`), e nao AT direto. A mudanca veio no commit `3032b1b`; o texto acima descreve a etapa anterior.
- Respostas da API seguem o Guia: 2xx/409 removem a mensagem da fila; 400/413 descartam so a mensagem rejeitada; 429 respeita o `Retry-After`; 5xx ou falta de resposta repetem com espera de 5 s, 15 s, 30 s e depois 60 s.
- 401/403/404: a mensagem e preservada e a telemetria pausada (decisao preliminar A3). A chave e verificada com `GET /iot/commands/{device_id}`, rota documentada do dispositivo, sem reenviar telemetria. A secao 7 do Guia manda descartar; a divergencia esta registrada como pendencia D1.
- O LittleFS nunca e formatado automaticamente (decisao A6). Uma falha de montagem e sinalizada e a montagem e tentada de novo a cada 30 s.
- A logica de decisao foi isolada em `PoliticaEnvio.cpp`, sem Arduino, para ser testada no computador.
- O `headerAvailable()` do ArduinoHttpClient 0.6.2 nao tem timeout e pode gravar 0xFF na linha; foi substituido por `CabecalhosHTTP.h`.
- LEDs: a troca de estado apaga os dois LEDs. O defeito do vermelho aceso depois de `ERRO` foi reproduzido por teste antes da correcao.

### Compilacao e testes

- `pio run -e esp32-c3-devkitm-1`: sucesso (RAM 12,2%, Flash 65,0%).
- `test/host/run_tests.ps1` (MSVC): 247 verificacoes, 0 falhas.
- O teste simulado de persistencia da fila nao foi feito (exigiria simular `String`, LittleFS e `Preferences`).
- Nenhum teste de placa, GPS, Wi-Fi, API real ou SIM800L foi executado com esta versao.
