# Checklist do MVP

Atualizado em 15/10/2026, com a validacao fisica relatada por Simao Bastos. Legenda: **implementado e compilado**, **testado fisicamente**, **preparado**, **nao implementado**, **pendente de confirmacao**.

| Item | Situacao | Evidencia ou pendencia |
| --- | --- | --- |
| Compilar ESP32-C3 | Concluido | `pio run -e esp32-c3-devkitm-1` com SUCCESS em 15/10/2026. |
| Testes no computador | Concluido | `test/host/run_tests.ps1`: 377 verificacoes, 0 falhas. Nao comprovam o hardware. |
| GPS com fix | Testado fisicamente | 15/10: posicao valida, 6 satelites, HDOP de cerca de 1,61 a 1,62 (relato de Simao Bastos). |
| Ultima posicao com `gps_ok:false` | Testado fisicamente (comportamento) | 15/10: comportamento aprovado na perda do GPS (E7). Nao ha sinal visual de erro do GPS nos LEDs: integracao nao implementada. |
| LEDs e estados | Parcialmente testado | 15/10: parcialmente aprovado (E5). LEDs apos erro e recuperacao (T4) nao testados. |
| Credenciais fora do codigo versionado | Preparado | `include/secrets.h` e ignorado; o provisionamento fica na maquina da apresentacao. |
| Contrato da API | Documentado | Resumos em `docs/integrations/api`; originais em `reference/`. |
| Envio HTTP por Wi-Fi | Testado fisicamente | 15/10: HTTP 202 "Telemetria recebida" e remocao da fila (E4). |
| Fila persistente e retentativa | Parcialmente testado | 15/10: inicializacao do LittleFS (E1), indisponibilidade e recuperacao da API (E6) e reinicializacao (T1) aprovadas. Persistencia da fila apos reinicio (T2) nao testada. |
| Tratamento de 401/403/404 | Implementado e compilado | Nao testado fisicamente (T3). Politica aguarda confirmacao (A3). |
| Pinout documentado | Concluido | `docs/hardware/PINOUT.md`; ligacoes informadas pelo responsavel pelo hardware. |
| Diagnostico serial SIM800L | Implementado; modem sem resposta | 15/10: o diagnostico mostrou "modem sem resposta". GSM nao validado. |
| Alimentacao e UART do SIM800L | Pendente | Fonte dedicada, nivel logico, SIM e antena nao validados. |
| Telemetria HTTP por GSM | Nao implementado | Caminho comentado em `HTTP.cpp`. |
| Bateria | Nao implementado | Diagnostico mostra -1,00 e o JSON envia 100 provisorio; nao e leitura real (T5). |
| Sensor de rompimento | Nao implementado | Sem pino definido. |
| Rele / solenoide | Nao implementado | Sem pino nem driver definidos. |
| Deep sleep | Nao implementado | Depende do sensor do lacre e da bateria. |
| Limite da fila | Nao implementado | Capacidade da flash nao medida. |
