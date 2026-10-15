# Notas de fontes e pontos em aberto

## Referencias consultadas

- `reference/ESP32-envio-de-dados.md` - versao 2.3, 12/10/2026 (cabecalho do arquivo).
- `reference/Guia-Firmware-Lacre-IoT.md` - o cabecalho diz versao 2.0 (10/10/2026), mas o historico do proprio arquivo vai ate a 2.3 (12/10/2026). A 2.1 foi aprovada por Nata da Silva Baracho em 10/10/2026. Divergencia registrada; o arquivo nao foi alterado.
- `reference/Regras-de-Negocio-e-Banco-Oxide.md`.
- `reference/Tipos-de-Erro.md` - versao 2.0, 10/10/2026.

Os arquivos acima foram preservados sem alteracao.

## Confirmado

A chave correta do firmware e `X-API-Key` do dispositivo. A `ADMIN_API_KEY` e usada somente em `X-Admin-Key` nas rotas administrativas. O backend nao cria dispositivos durante uma telemetria. Bateria e posicao sao obrigatorias para a API aceitar mensagens.

## Pendencias e responsaveis

- Chave, identificador e URL de homologacao do dispositivo: responsavel pelo cadastro/backend.
- Provisionamento seguro definitivo e HTTPS: equipe de backend e seguranca.
- Intervalos de telemetria e comandos: produto/backend.
- APN, operadora, taxa UART efetiva e ativacao de dados do SIM800L: validacao de bancada pelo time de hardware.
- Alimentacao do modem, solenoide, rele e sensor de rompimento: validacao eletrica; nenhum parametro foi assumido neste firmware.
