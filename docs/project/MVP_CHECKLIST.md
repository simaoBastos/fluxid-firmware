# Checklist do MVP

| Item | Situacao | Evidencia ou pendencia |
| --- | --- | --- |
| Compilar ESP32-C3 | Concluido nesta etapa | `pio run -e esp32-c3-devkitm-1 --silent` terminou com sucesso apos as alteracoes. |
| GPS com fix | Validacao anterior informada | Nao repetida nesta etapa. |
| LEDs e estados | Validacao anterior informada | Nao repetida nesta etapa. |
| Credenciais fora do codigo versionado | Preparado | `include/secrets.h` e ignorado; valores provisionados ainda faltam. |
| Contrato da API | Documentado | Resumos derivados dos quatro arquivos em `integrations/api/reference`. |
| Diagnostico serial SIM800L | Implementado, pendente de bancada | AT, SIM, registro, sinal e estados de pacote consultados com timeout. |
| Alimentacao e UART do SIM800L | Pendente | Exige circuito energizado, SIM e taxa UART confirmada. |
| Dados moveis SIM800L | Pendente | APN, operadora e PDP ativo nao foram validados. |
| Telemetria HTTP por GSM | Fora do escopo | Nao implementada nesta etapa. |
| Fila persistente e retentativa contratual | Pendente | Necessaria antes de habilitar envio real. |
| Bateria, lacre, rele e solenoide | Pendente | Sem especificacao eletrica confirmada ou leitura implementada. |
