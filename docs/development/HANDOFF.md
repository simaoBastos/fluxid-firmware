# Handoff da etapa atual

Atualizado em 15/10/2026.

## Situacao

O Ciclo 01 foi mesclado no `main` pelo PR [#1](https://github.com/simaoBastos/fluxid-firmware/pull/1) (merge `6af9529`, conteudo do commit `9ac4093`). Validacao fisica parcial relatada por Simao Bastos em 15/10/2026: aprovados E1 a E4, E6 e T1; E5 parcial; E7 aprovado quanto ao comportamento; T2 a T5 nao testados. O Registro 07 foi concluido em `docs/ai-records/ciclos/Registro_07_Ciclo01_Fila_HTTP_LEDs_FluxID.docx`.

### Diagnostico: sinal de erro do GPS nos LEDs (15/10, sem alteracao de codigo)

- `ReproduzirCodigoErro()` (`src/LED.cpp`) existe e reproduz um unico codigo, CURTO-CURTO-LONGO-CURTO, e so roda no estado `ERRO`.
- A perda do GPS so muda `gps_ok` para false (`src/GPS.cpp`); nenhum codigo muda `estadoAtual` por causa do GPS.
- `CalcularEstadoSinalizado()` (`src/PoliticaEnvio.cpp`) so usa a fila indisponivel (`ERRO`) e a pausa 401/403/404 (`NAO_CADASTRADO`/`DESATIVADO`). Com o GPS sem fix, o estado continua `OPERACIONAL` (verde piscando), o que explica a falta de sinal visual.
- Nao ha defeito na rotina; falta a integracao, que exige decidir qual estado ou padrao usar (o `ERRO` gera evento `hardware_failure` segundo o Guia, secao 5).
- `AtualizarLEDs()` roda uma vez por volta do `loop()`; durante uma requisicao HTTP bloqueante o padrao congela e depois continua.

## Alteracoes desta etapa

- `src/PoliticaEnvio.h/.cpp` (novo): respostas HTTP, esperas de 5 s a 60 s, `Retry-After`, pausa por 401/403/404 com verificacao da chave e sinalizacao nos LEDs.
- `src/CabecalhosHTTP.h` (novo): leitura dos cabecalhos com limite de tempo, porque o `headerAvailable()` do ArduinoHttpClient 0.6.2 nao tem timeout.
- `src/HTTP.cpp/.h`: timeout de resposta de 10 s e `VerificarAutorizacaoAPI()` (`GET /iot/commands/{device_id}`).
- `src/FilaOffline.cpp/.h`: descarte so em 400/413, regravacao do `attempt_count`, sem formatacao automatica e montagem repetivel.
- `src/main.cpp`: usa a politica de envio, tenta montar o LittleFS de novo a cada 30 s e envia telemetria depois do primeiro fix com `gps_ok:false`.
- `src/LED.cpp`: troca de estado apaga os dois LEDs; padroes para `DESATIVADO` e `MANUTENCAO`; `ERRO` usa so o vermelho.
- `test/host/`: testes no computador (MSVC).
- `docs/hardware/PINOUT.md` e `README.md`.

## Comandos executados

- `pio run -e esp32-c3-devkitm-1`: SUCCESS (RAM 12,2%, Flash 65,0%).
- `powershell -ExecutionPolicy Bypass -File test\host\run_tests.ps1`: 377 verificacoes, 0 falhas.
- `git diff --check`: sem problemas.

## Bloqueios e proxima revisao humana

- Preencher `include/secrets.h` na maquina da apresentacao: Wi-Fi, URL da API, chave e `device_id`.
- Fazer os testes de bancada da parte B do questionario.
- Revisao corretiva de 15/10 (nao commitada): 401/403/404 descartam so a mensagem rejeitada (secao 7) e mantem a pausa; o `Retry-After` e respeitado sem o teto de 15 min; o leitor de cabecalhos sai na hora se a conexao cair e trata `Retry-After` ilegivel (60 s).
- Medir na bancada a perda de NMEA durante bloqueios do HTTP antes de mexer no buffer da UART do GPS.
- **Nao rodar `erase` nem `uploadfs` na placa da apresentacao:** os dois apagam a fila.
- Testes fisicos pendentes: T2 (persistencia da fila apos reinicio), T3 (401/403/404), T4 (LEDs apos erro e recuperacao), T5 (bateria) e a sinalizacao visual da perda do GPS.
- Questionario do Ciclo 01: partes A e C ainda sem resposta formal.
