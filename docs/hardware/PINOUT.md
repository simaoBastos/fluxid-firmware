# Pinout do FluxID (ESP32-C3 DevKitM-1)

Registro dos pinos usados pelo firmware e das ligações físicas da montagem do protótipo.

- **Configurado no firmware:** o pino aparece no código (conferido em 15/10/2026).
- **Ligação física:** como o componente está ligado na placa. Os dados abaixo foram **informados pelo responsável pelo hardware em 15/10/2026**. A equipe de software não fez inspeção ou medição elétrica.
- **Teste físico:** só conta o que foi observado funcionando na placa. Compilar não é teste físico.

## Tabela de pinos

| Pino | Componente | Sinal do componente | Direção (vista do ESP32-C3) | Função | Configurado no firmware | Ligação física | Teste físico |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 3V3 | GPS | VCC | Saída de alimentação | Alimenta o módulo GPS | Não se aplica | Informada | Não registrado separadamente |
| GND | Todos | GND | Referência | Terra comum dos componentes | Não se aplica | Informada | Não registrado separadamente |
| GPIO0 | LED verde | Entrada do LED (montagem não detalhada) | Saída digital | Padrões de estado | Sim: `InicializarLEDs(0, 1)` em [main.cpp](../../src/main.cpp), [LED.cpp](../../src/LED.cpp) | Informada e confirmada pelo responsável | Funcionamento informado pela equipe (Registro 01). Os padrões novos de 15/10 **ainda não foram vistos na placa**. |
| GPIO1 | LED vermelho | Entrada do LED (montagem não detalhada) | Saída digital | Padrões de estado e código de erro | Sim: idem | Informada e confirmada pelo responsável | Idem |
| GPIO6 | SIM800L | TXD | Entrada (RX do ESP32-C3) | Recebe dados do modem | Sim: `InicializarGSM(6, 7)`, UART0 remapeada (`HardwareSerial SerialSIM800(0)`) em [GSM.cpp](../../src/GSM.cpp), 9600 baud | Informada | **Não testado**: nenhuma resposta AT registrada |
| GPIO7 | SIM800L | RXD | Saída (TX do ESP32-C3) | Envia comandos AT ao modem | Sim: idem | Informada | **Não testado** |
| GPIO20 | GPS | TX | Entrada (RX do ESP32-C3) | Recebe NMEA | Sim: `InicializarGPS(20, 21)`, UART1 (`HardwareSerial SerialGPS(1)`) em [GPS.cpp](../../src/GPS.cpp), 9600 baud | Informada | Fix real informado pela equipe ([PROJECT_CONTEXT.md](../project/PROJECT_CONTEXT.md)) |
| GPIO21 | GPS | RX | Saída (TX do ESP32-C3) | Linha de envio ao GPS (o firmware não envia comandos) | Sim: idem | Informada | Não se aplica: o firmware não transmite ao GPS |

O firmware acende os LEDs com `HIGH` (ver `LED.cpp`). O resistor, o seu valor e a forma exata da ligação não foram registrados.

## Pinos reservados ou ocupados pela placa

| Pino | Situação |
| --- | --- |
| GPIO18 / GPIO19 | USB D− / D+ (monitor serial por USB CDC, `ARDUINO_USB_CDC_ON_BOOT=1`). Não usar. |
| GPIO2, GPIO8, GPIO9 | Pinos de strapping do ESP32-C3. O GPIO9 é o botão BOOT da DevKitM-1. Não usados pelo firmware. |
| GPIO3, GPIO4, GPIO5, GPIO10 | Livres no firmware atual. GPIO0 a GPIO5 são os únicos que acordam o C3 do deep sleep, e GPIO0 a GPIO4 têm ADC1 (útil para a bateria). Reservar com cuidado para o sensor do lacre e a bateria. |

As informações desta seção vêm da documentação da Espressif para o ESP32-C3 e a DevKitM-1, não de medição na montagem.

## Estado por função

| Função | Implementado | Compilado | Testado fisicamente |
| --- | --- | --- | --- |
| LEDs verde/vermelho | Sim | Sim | Padrões antigos: informado. Padrões de 15/10: **pendente** |
| GPS (UART1) | Sim | Sim | Fix informado pela equipe |
| SIM800L (UART0, diagnóstico AT via TinyGSM) | Sim | Sim | **Pendente.** Configurar a UART não prova que o modem responde, e menos ainda que a rede móvel funciona. O envio HTTP por GPRS está desativado em [HTTP.cpp](../../src/HTTP.cpp). |
| Sensor de rompimento do lacre | Não | Não | Não implementado; sem pino definido |
| Relé / solenoide | Não | Não | Não implementado; sem pino definido |
| Medição de bateria | Não | Não | Não implementado; o JSON envia um valor provisório |

## Observações elétricas (não verificadas)

Pendências para conferir na bancada. **Nenhum item abaixo foi medido ou validado pela equipe de software.**

- **GPS no 3V3:** informado pelo responsável. Conferir se o módulo GPS usado aceita 3,3 V na entrada de alimentação e se o 3V3 da DevKitM-1 dá conta da corrente do GPS mais a do restante.
- **GND comum:** informado. Necessário para as duas UARTs funcionarem.
- **SIM800L, alimentação:** o datasheet da SIMCom indica alimentação entre 3,4 V e 4,4 V e picos de cerca de 2 A durante a transmissão. **Não alimentar pelo 3V3 da placa.** É preciso uma fonte dedicada com capacitor de reserva perto do modem. A fonte usada na montagem não foi registrada.
- **SIM800L, nível lógico:** a UART do modem trabalha em torno de 2,8 V. Conferir se o TX do ESP32-C3 (3,3 V) para o RXD do modem precisa de divisor ou conversor, e se o TXD do modem é lido corretamente pelo ESP32-C3. Pendente.
- **SIM800L, antena e SIM:** sem registro de teste.
- **GPIO20/21:** são os pinos padrão da UART0 do C3 (U0RXD/U0TXD). No reset, a ROM pode imprimir mensagens de boot no GPIO21, que vai para o RX do GPS. Em geral é inofensivo; não foi verificado.
- **LEDs:** o valor dos resistores e a corrente por GPIO não foram registrados.

## Manutenção deste documento

Sempre que uma alteração aprovada mudar um GPIO ou a função dele no firmware, este arquivo deve ser atualizado no mesmo conjunto de mudanças. Um pino só passa a "ligação física confirmada" quando o responsável pelo hardware confirmar; não basta estar definido no código.
