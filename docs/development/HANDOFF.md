# Handoff da etapa atual

Atualizado em 15/10/2026.

## Situacao

O Ciclo 01 esta no PR [#1](https://github.com/simaoBastos/fluxid-firmware/pull/1) (`fix/fila-contrato-api`), aguardando a validacao humana. Tudo foi compilado e testado em software; **nada foi testado na placa ou contra a API real com esta versao**.

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
- Registro 07 em rascunho (`docs/ai-records/ciclos/Registro_07_Ciclo01_Fila_HTTP_LEDs_FluxID_RASCUNHO.docx`), com validacao humana e decisao final pendentes. Depois da aprovacao: inserir o validador, completar esses campos e remover a marca de rascunho.
