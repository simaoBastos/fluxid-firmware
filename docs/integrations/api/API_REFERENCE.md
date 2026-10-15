# Referencia tecnica para a futura integracao

## Telemetria

`POST /iot/telemetries` recebe JSON com `message_id`, `device_id`, `latitude`, `longitude` e `battery_percent`. Campos opcionais confirmados incluem `gps_ok`, `seal_status` (`LOCKED`, `UNLOCKED`, `BROKEN`), `speed_kmh`, `gsm_signal`, `satelites` ou `satellites`, `hdop`, `device_state` e `attempt_count`.

O firmware envia esse corpo por Wi-Fi (`HTTP.cpp`). Bateria (valor provisorio 100), lacre e GSM ainda nao sao leituras fisicas; `seal_status` so e enviado com valor valido e `gsm_signal` so quando o modem responde.

## Outras rotas previstas

- `POST /iot/activations`: ativacao ou desativacao, com posicao e bateria.
- `POST /iot/events`: eventos como `startup` e `seal_changed`.
- `POST /iot/alerts`: alertas que o firmware identifica.
- `GET /iot/commands/{device_id}` e `POST /iot/commands/confirm`: comandos de valvula, fora do escopo atual. O firmware usa o `GET` apenas para verificar a chave durante uma pausa por 401/403/404; os comandos listados sao ignorados e nenhum e confirmado.

## Semantica de respostas

`409` representa mensagem previamente recebida e equivale a sucesso para a fila local. Sem resposta ou com `500`, o firmware conserva a mesma mensagem e repete com espera crescente; em `429`, respeita o `Retry-After`. A implementacao esta em `src/PoliticaEnvio.cpp`.
