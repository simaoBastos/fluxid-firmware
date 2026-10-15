# Contrato da API do firmware

Fonte principal: `reference/Guia-Firmware-Lacre-IoT.md`. O cabecalho do arquivo diz versao 2.0, e o historico interno vai ate a 2.3 (12/10/2026); ver `SOURCE_NOTES.md`. Este resumo nao substitui os documentos de referencia.

## Requisitos confirmados

- Base das rotas: `http://<IP_DA_API>:3000/api/v1`; HTTPS esta pendente para producao.
- O firmware usa `Content-Type: application/json` e a `X-API-Key` do proprio dispositivo. `X-Admin-Key` e exclusiva das rotas da equipe e nao pertence ao firmware.
- A API nao cadastra dispositivos automaticamente. Cadastro e chave dependem do responsavel pelo backend.
- Telemetria usa `POST /iot/telemetries`. `message_id`, `device_id`, `latitude`, `longitude` e `battery_percent` sao obrigatorios.
- Cada mensagem nova recebe um identificador unico. Reenvios mantem o mesmo identificador e incrementam `attempt_count`.
- `200`, `201`, `202` e `409` confirmam entrega. `400`, `401`, `403`, `404` e `413` nao devem ser reenviados sem correcao. `429` usa `Retry-After`; `500` ou falta de resposta exigem fila e retentativa.
- Sem GPS, depois de uma primeira posicao valida, enviar a ultima posicao com `gps_ok: false`. Antes da primeira posicao valida, nao enviar.

## Como o firmware atende (15/10/2026)

| Regra | Implementacao | Situacao |
| --- | --- | --- |
| 200/201/202/409 confirmam | Remove da fila | Conforme |
| 400/413 nao reenviar | Registra e descarta so essa mensagem | Conforme |
| 401/403/404 nao insistir | Preserva a fila, pausa a telemetria e verifica a chave com `GET /iot/commands/{device_id}` (30 s, 60 s, 120 s, depois 5 min) | Conforme as secoes 5 e 6; **diverge da secao 7**, que manda descartar (pendencia D1) |
| 429 com `Retry-After` | Espera o valor informado, com teto de 15 min | Teto pendente de decisao (D2) |
| 500 ou sem resposta: mesmo `message_id` e `attempt_count + 1` | Espera de 5 s, 15 s, 30 s e depois 60 s; +1 so em tentativa realizada | Conforme |
| Sem GPS depois do primeiro fix: ultima posicao com `gps_ok:false` | Implementado | Conforme; bancada pendente |
| `battery_percent` obrigatorio | Enviado com valor provisorio 100 | **Nao e medicao real** |

Tudo acima foi compilado e testado em software; nada foi testado contra a API real com esta versao.

## Fora desta etapa

O firmware ainda nao implementa ativacao, eventos, alertas, comandos, HTTPS ou envio HTTP por GSM. Nao cria dispositivos nem altera a API.
