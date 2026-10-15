# Questionário de validação humana: Ciclo 01 (revisão 3)

**Tarefa:** fila offline e envio HTTP conforme o contrato da API (Guia-Firmware-Lacre-IoT, seções 5, 6 e 7), LEDs, timeouts e pinout
**Branch:** `fix/fila-contrato-api`. PR: [#1](https://github.com/simaoBastos/fluxid-firmware/pull/1), aberto em 15/10/2026 a pedido da equipe e **ainda não mesclado**.
**Validador:** _pendente: o nome só é inserido após a aprovação final_
**Situação:** **em validação**. Testado só em software; nenhum teste na placa ou contra a API real.

## Histórico das revisões

| Revisão | O que mudou |
| --- | --- |
| 1 | Primeira versão: 4xx descartavam, espera de 10 s a 5 min e formatação automática do LittleFS. |
| 2 | Decisões preliminares A3 e A6: 401/403/404 preservam e pausam (15 min); sem formatação automática. |
| 3 | Espera de 5 s, 15 s, 30 s e depois 60 s; pausa com verificação da chave por `GET /iot/commands/{device_id}` em vez dos 15 min fixos; timeouts HTTP; correção dos LEDs; testes no computador; `PINOUT.md`. |

## Validação técnica (feita pela IA)

| Verificação | Resultado real |
| --- | --- |
| `pio run -e esp32-c3-devkitm-1` no `main` (antes) | SUCCESS: RAM 12,2%, Flash 64,3% |
| `pio run -e esp32-c3-devkitm-1` (revisão 3) | SUCCESS, sem warnings ou erros em `src/`; RAM 12,2% (39.988 B), Flash 65,0% (852.614 B) |
| `test/host/run_tests.ps1` (MSVC, no computador) | `test_politica_envio`: 127 verificações; `test_cabecalhos_http`: 17; `test_leds`: 103. **0 falhas.** |
| `git diff --check` | Sem problemas |
| Teste do `test_leds` contra o `LED.cpp` antigo | **Reproduziu o defeito**: o vermelho ficava aceso depois de `ERRO → OPERACIONAL` |
| Persistência da fila em LittleFS | **Sem teste simulado** (exigiria simular `String`, LittleFS e `Preferences` inteiros); só revisão do código |
| `pio run -t buildfs` sem `data/` | FAILED; com `data/` vazia, SUCCESS. Documentado no README. |
| Placa, GPS, Wi-Fi, API real, SIM800L | **Não realizado** (parte B) |

Os testes no computador cobrem a lógica isolada (decisões, esperas, cabeçalhos, LEDs). Eles não comprovam o hardware, a rede, o LittleFS nem a API.

## Classificação HTTP comparada ao Guia

Implementação: `ClassificarRespostaHTTP` e `DecidirAposEnvio` em [src/PoliticaEnvio.cpp](../../../src/PoliticaEnvio.cpp). Guia: [Guia-Firmware-Lacre-IoT.md](../../integrations/api/reference/Guia-Firmware-Lacre-IoT.md).

| Código | Guia, seção 6 | Guia, seções 5 e 7 | Firmware (revisão 3) | Conforme? |
| --- | --- | --- | --- | --- |
| 200, 201, 202, 409 | Apagar da fila | Seção 7: só sai com esses códigos | Remove; volta ao intervalo de 2 s | Sim |
| 400, 413 | Não reenviar; registrar | Seção 7: descartar | Registra o código e o conteúdo no serial; descarta só essa mensagem | Sim |
| 401, 403, 404 | Conferir a chave/cadastro; **não insistir** | Seção 5: estados `NAO_CADASTRADO`/`DESATIVADO`. Seção 7: **descartar** | Preserva a fila e pausa a telemetria; verifica a chave com `GET /iot/commands/{device_id}` em 30 s, 60 s, 120 s e depois a cada 5 min; só um `200` retoma | Sim pelas seções 5 e 6; **diverge da seção 7** (pendência D1) |
| 429 | Esperar o `Retry-After`, mesmo `message_id` | Janelas de 1 min | Espera o `Retry-After` (teto de 15 min); +1 no `attempt_count` | Sim; o teto é a pendência D2 |
| 5xx / sem resposta | Reenviar com `attempt_count + 1` | Seção 7: espera crescente (exemplo) | 5 s → 15 s → 30 s → 60 s (fica em 60 s); +1 | Sim |
| Outros (204, 3xx, 405, 422…) | Não previstos | Não previstos | Mantém e repete com espera crescente | Escolha conservadora |

## Revisão de recuperabilidade

| Cenário | Comportamento | Dados preservados? |
| --- | --- | --- |
| Rede ou API fora do ar | Espera crescente até 60 s; a mensagem fica no LittleFS | Sim |
| API volta | Retoma em até 60 s; envia em ordem | Sim |
| Reinício antes da remoção | Reenvia; o `409` é tratado como sucesso | Sim, sem duplicar |
| Reinício durante a regravação do `attempt_count` | O `.tmp` é apagado no boot; o original continua intacto | Sim; o `attempt_count` pode ficar 1 abaixo do real |
| Chave/cadastro recusados | Pausa; só consultas `GET`, sem reenviar telemetria; a fila continua crescendo | Sim |
| Reinício durante a pausa | A pausa é esquecida e o envio é tentado logo após o boot | Sim |
| LittleFS não monta | Nada é formatado; LED `ERRO`; tenta montar de novo a cada 30 s | Sim, o que estava gravado; as telemetrias novas não são geradas |
| Flash cheia | A gravação falha (`ERRO: telemetria nao foi persistida.`) | As antigas sim; as novas se perdem |
| Requisição sem resposta | Bloqueia o loop por cerca de 13 s (IP) ou até cerca de 28 s (nome de host, com DNS) | Sim; frases NMEA são perdidas durante o bloqueio |

## LEDs (revisão 3)

| Estado | Verde | Vermelho | Acionado por |
| --- | --- | --- | --- |
| `OPERACIONAL` | Pisca 100 ms a cada 4 s | Apagado | Normal |
| `NAO_CADASTRADO` | Aceso | Apagado | Pausa por `401`/`404` |
| `DESATIVADO` | Apagado | Pisca 1 s / 1 s | Pausa por `403` |
| `ERRO` | Apagado | Curto-curto-longo-curto | LittleFS indisponível |
| `LACRE_ROMPIDO` | Apagado | Aceso | Nada (sem sensor) |
| `MANUTENCAO` | Alterna | Alterna | Nada (sem modo de manutenção) |

## Parte A: revisão do código

Marque S ou N.

| # | Pergunta | S/N | Comentário |
| --- | --- | --- | --- |
| A1 | O PR #1 contém apenas o escopo combinado (fila, contrato HTTP, timeouts, LEDs, testes, pinout e README)? | | |
| A2 | A tabela de classificação acima está correta? | | |
| A3 | Usar `GET /iot/commands/{device_id}` para verificar a chave durante a pausa é aceitável? Efeito colateral: o servidor conta a consulta como contato do lacre. | | |
| A3b | A sequência de verificação (30 s, 60 s, 120 s e depois 5 min) atende à apresentação sem "insistir"? | | |
| A4 | O +1 acontece só em tentativa realizada e o `message_id` nunca muda? | | |
| A5 | A espera de 5 s, 15 s, 30 s e depois 60 s está adequada? | | |
| A6 | Montar o LittleFS sem formatar e tentar de novo a cada 30 s atende? | | |
| A7 | O código segue o estilo do projeto? | | |
| A8 | Códigos não previstos ficam em "repetir", e não em "descartar". Concorda? | | |
| A9 | Os padrões de LED da tabela acima estão claros e adequados? | | |
| A10 | Timeout de resposta de 10 s, cabeçalhos com 3 s e o novo leitor de cabeçalhos estão adequados? | | |
| A11 | O `PINOUT.md` está correto e não afirma nada que não foi validado? | | |

## Parte B: testes de bancada (nenhum realizado)

Grave com `pio run -t upload` e acompanhe com `pio device monitor`. **Não rode `erase` nem `uploadfs` na placa da apresentação: os dois apagam a fila.**

| # | Teste | Como fazer | Resultado esperado | Situação |
| --- | --- | --- | --- | --- |
| B1 | Envio normal | Wi-Fi, API e chave corretos, com fix | `HTTP status: 202`, `Telemetria removida da fila`; LED verde piscando e vermelho apagado | Pendente |
| B2 | Sem resposta | API desligada | `Proxima tentativa em (s): 5`, depois 15, 30, 60, 60; `attempt_count atualizado para N` uma vez por tentativa | Pendente |
| B3 | Sem Wi-Fi | Desligue o roteador | Nenhum envio é tentado; o `attempt_count` não muda | Pendente |
| B4 | Recuperação | Religue a API | Retoma em até 60 s; envio em ordem com o mesmo `message_id` e `attempt_count` maior que 0 | Pendente |
| B5 | Chave errada | `FLUXID_DEVICE_API_KEY` incorreta | `HTTP status: 401`; `Envio PAUSADO (HTTP 401)`; LED verde fixo; `Verificando chave e cadastro na API...` em 30, 60, 120 s e depois a cada 300 s; nenhuma telemetria reenviada; a fila só cresce | Pendente |
| B6 | Recuperação da chave | Regrave com a chave correta e reinicie, ou corrija o cadastro e aguarde a verificação | `Autorizacao confirmada; envios retomados.` (ou envio logo após o boot); a fila esvazia | Pendente |
| B7 | Corpo inválido (400) | Comente temporariamente `doc["battery_percent"]` em `JSON.cpp` | `HTTP status: 400`; `Telemetria descartada (HTTP 400)` com o conteúdo; a próxima mensagem é enviada | Pendente |
| B8 | Perda de fix | Depois do fix, desconecte o GPS | O JSON sai com `"gps_ok":false` e a última posição | Pendente |
| B9 | Antes do primeiro fix | Reinicie sem sinal de GPS | Nenhuma telemetria; `Posicao utilizavel: Nao` | Pendente |
| B10 | Reinício com fila | Reinicie com mensagens na fila | Contagem e ordem preservadas; o envio retoma | Pendente |
| B11 | Bloqueio do HTTP | API num IP que aceita conexão mas não responde, ou desligada | Medir quanto tempo o loop trava (esperado: até cerca de 13 s) | Pendente |
| B12 | LittleFS indisponível | **Só numa placa de teste:** `pio run -t erase` e depois `upload` | `Fila offline: INDISPONIVEL`; LED vermelho com o código de erro; nenhum dado apagado pelo firmware | Pendente |
| B13 | 429 | Só com uma API simulada ou ultrapassando o limite | `Retry-After (s): N`; próxima tentativa em N s | Pendente; difícil de reproduzir |

## Parte D: pendências de decisão (sem mudança na API)

1. **D1:** com 401, 403 e 404, a mensagem rejeitada deve ser **descartada** (Guia, seção 7 e exemplo de código) ou **preservada sem insistir** (seções 5 e 6, decisão A3 da revisão 2)? Hoje ela é preservada.
2. **D2:** manter, aumentar ou remover o teto de 15 min do `Retry-After`? Hoje um valor maior é reduzido.
3. **D3:** o `Retry-After` da API é sempre em segundos? O valor em formato de data é ignorado e cai na espera crescente.

## Parte C: decisão final

- [ ] Aprovado: o PR #1 pode ser mesclado
- [ ] Aprovado com ressalvas
- [ ] Reprovado

Observações:

_______________________________________________________________
