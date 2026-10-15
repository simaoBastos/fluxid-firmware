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
| 401/403/404: descartar (secao 7) e nao insistir (secoes 5 e 6) | Descarta so a mensagem rejeitada, pausa as demais e verifica a chave com `GET /iot/commands/{device_id}`. Um 200 libera uma telemetria; so a aceitacao dela encerra a pausa. Esperas de 30 s, 60 s, 120 s e depois 5 min | Conforme pela leitura do Guia; aguarda confirmacao humana (A3) |
| 429 com `Retry-After` | Espera o valor informado integralmente (limite do temporizador: ~49,7 dias); ilegivel: 60 s | Conforme; a espera nao sobrevive a um reinicio |
| 500 ou sem resposta: mesmo `message_id` e `attempt_count + 1` | Espera de 5 s, 15 s, 30 s e depois 60 s; +1 so em tentativa realizada | Conforme |
| Sem GPS depois do primeiro fix: ultima posicao com `gps_ok:false` | Implementado | Conforme; bancada pendente |
| `battery_percent` obrigatorio | Enviado com valor provisorio 100 | **Nao e medicao real** |

Tudo acima foi compilado e testado em software; nada foi testado contra a API real com esta versao.

## Fora desta etapa

O firmware ainda nao implementa ativacao, eventos, alertas, comandos, HTTPS ou envio HTTP por GSM. Nao cria dispositivos nem altera a API.
