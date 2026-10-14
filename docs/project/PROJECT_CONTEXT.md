# Contexto atual do FluxID firmware

O projeto usa PlatformIO, Arduino e ESP32-C3 DevKitM-1. `platformio.ini` declara TinyGPSPlus e ArduinoJson; o ambiente compilado nesta etapa usa Espressif32 7.1.3, ArduinoJson 7.4.3 e TinyGPSPlus 1.1.0.

## Modulos existentes

- GPS: UART dedicada, parsing de localizacao, velocidade e satelites. O GPS teve fix real informado pelo projeto, mas esta execucao nao realizou teste fisico.
- Wi-Fi: inicializacao e diagnostico simples; passa a ler configuracao local ignorada pelo Git.
- JSON e HTTP: serializacao parcial e POST hoje desabilitado. Nao ha fila persistente nem leitura de corpo HTTP.
- Mensagem: contador NVS com identificador `MSG-*`; retentativa e fila ainda nao atendem todo o contrato.
- Estado e LEDs: estados e padroes existentes preservados; a validacao fisica anterior foi informada, nao repetida aqui.
- Dispositivo: bateria e lacre ainda usam dados fixos; suas rotinas de leitura estao vazias.
- GSM: diagnostico serial AT nao bloqueante implementado nesta etapa; nao envia HTTP e nao configura dados.

## Limitacoes relevantes

O GPS usa GPIO 20/21 e os LEDs usam GPIO 0/1 no codigo existente. O SIM800L usa o planejamento GPIO 6 (RX do ESP32) e GPIO 7 (TX do ESP32), ambos expostos na DevKitM-1 e sem classificacao de strapping na documentacao da Espressif. A alimentacao final do modem e os demais circuitos continuam pendentes de validacao fisica.
