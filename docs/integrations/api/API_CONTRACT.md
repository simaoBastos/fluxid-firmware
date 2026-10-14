# Contrato da API do firmware

Fonte principal: `reference/Guia-Firmware-Lacre-IoT.md`, versao 2.2, aprovada por Nata da Silva Baracho em 11/10/2026. Este resumo nao substitui os documentos de referencia.

## Requisitos confirmados

- Base das rotas: `http://<IP_DA_API>:3000/api/v1`; HTTPS esta pendente para producao.
- O firmware usa `Content-Type: application/json` e a `X-API-Key` do proprio dispositivo. `X-Admin-Key` e exclusiva das rotas da equipe e nao pertence ao firmware.
- A API nao cadastra dispositivos automaticamente. Cadastro e chave dependem do responsavel pelo backend.
- Telemetria usa `POST /iot/telemetries`. `message_id`, `device_id`, `latitude`, `longitude` e `battery_percent` sao obrigatorios.
- Cada mensagem nova recebe um identificador unico. Reenvios mantem o mesmo identificador e incrementam `attempt_count`.
- `200`, `201`, `202` e `409` confirmam entrega. `400`, `401`, `403`, `404` e `413` nao devem ser reenviados sem correcao. `429` usa `Retry-After`; `500` ou falta de resposta exigem fila e retentativa.
- Sem GPS, depois de uma primeira posicao valida, enviar a ultima posicao com `gps_ok: false`. Antes da primeira posicao valida, nao enviar.

## Fora desta etapa

O firmware ainda nao implementa fila persistente, ativacao, eventos, alertas, comandos, HTTPS ou envio HTTP por GSM. Esta etapa tambem nao cria dispositivos nem altera a API.
