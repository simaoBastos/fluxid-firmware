# Referencia tecnica para a futura integracao

## Telemetria

`POST /iot/telemetries` recebe JSON com `message_id`, `device_id`, `latitude`, `longitude` e `battery_percent`. Campos opcionais confirmados incluem `gps_ok`, `seal_status` (`LOCKED`, `UNLOCKED`, `BROKEN`), `speed_kmh`, `gsm_signal`, `satelites` ou `satellites`, `hdop`, `device_state` e `attempt_count`.

O firmware atual monta parte desse corpo, mas bateria, lacre e GSM ainda nao sao leituras fisicas. O envio permanece desabilitado e nao foi alterado nesta etapa.

## Outras rotas previstas

- `POST /iot/activations`: ativacao ou desativacao, com posicao e bateria.
- `POST /iot/events`: eventos como `startup` e `seal_changed`.
- `POST /iot/alerts`: alertas que o firmware identifica.
- `GET /iot/commands/{device_id}` e `POST /iot/commands/confirm`: comandos de valvula, fora do escopo atual.

## Semantica de respostas

`409` representa mensagem previamente recebida e equivale a sucesso para a fila local. O firmware deve conservar a mesma mensagem quando nao houver resposta ou houver `500`; a proxima etapa de transporte deve tambem respeitar `Retry-After` em `429`.
