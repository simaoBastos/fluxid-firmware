# Checklist do MVP

Atualizado em 15/10/2026. Legenda: **implementado e compilado**, **testado fisicamente**, **preparado**, **nao implementado**, **pendente de confirmacao**.

| Item | Situacao | Evidencia ou pendencia |
| --- | --- | --- |
| Compilar ESP32-C3 | Concluido | `pio run -e esp32-c3-devkitm-1` com SUCCESS em 15/10/2026. |
| Testes no computador | Concluido | `test/host/run_tests.ps1`: 377 verificacoes, 0 falhas. Nao comprovam o hardware. |
| GPS com fix | Testado fisicamente (versao anterior) | Fix real informado pela equipe; nao repetido com a branch atual. |
| Ultima posicao com `gps_ok:false` | Implementado e compilado | Bancada pendente (questionario, B8). |
| LEDs e estados | Implementado e compilado | Padroes novos de 15/10 nao vistos na placa. A versao anterior foi validada pela equipe (Registro 01). |
| Credenciais fora do codigo versionado | Preparado | `include/secrets.h` e ignorado; o provisionamento fica na maquina da apresentacao. |
| Contrato da API | Documentado | Resumos em `docs/integrations/api`; originais em `reference/`. |
| Envio HTTP por Wi-Fi | Testado fisicamente (versao anterior) | HTTP 202 e remocao da fila observados (Registro 06), com falhas intermitentes -3. A branch atual nao foi testada. |
| Fila persistente e retentativa | Implementado e compilado | LittleFS, espera de 5 s a 60 s, `Retry-After`, `attempt_count`. Bancada pendente (B2, B4, B10). |
| Tratamento de 401/403/404 | Implementado e compilado | Descarta so a mensagem rejeitada e pausa as demais; verificacao por `GET /iot/commands`. Politica aguarda confirmacao (A3); bancada e API real pendentes. |
| Pinout documentado | Concluido | `docs/hardware/PINOUT.md`; ligacoes informadas pelo responsavel pelo hardware. |
| Diagnostico serial SIM800L | Implementado, pendente de bancada | Nenhuma resposta AT registrada. |
| Alimentacao e UART do SIM800L | Pendente | Fonte dedicada, nivel logico, SIM e antena nao validados. |
| Telemetria HTTP por GSM | Nao implementado | Caminho comentado em `HTTP.cpp`. |
| Bateria | Nao implementado | O JSON envia 100 provisorio; sem ADC nem divisor. |
| Sensor de rompimento | Nao implementado | Sem pino definido. |
| Rele / solenoide | Nao implementado | Sem pino nem driver definidos. |
| Deep sleep | Nao implementado | Depende do sensor do lacre e da bateria. |
| Limite da fila | Nao implementado | Capacidade da flash nao medida. |
