
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoHttpClient.h>

#include <HTTP.h>
#include <JSON.h>
#include <Mensagem.h>
#include <GSM.h>
#include <secrets.h>

namespace
{
    struct EnderecoAPI
    {
        String host;
        String caminhoBase;
        uint16_t porta;
        bool valido;
    };

    EnderecoAPI InterpretarURL()
    {
        EnderecoAPI endereco;
        endereco.host = "";
        endereco.caminhoBase = "";
        endereco.porta = 80;
        endereco.valido = false;

        String url = FLUXID_API_BASE_URL;

        // Esta implementação utiliza HTTP, não HTTPS.
        if (!url.startsWith("http://"))
            return endereco;

        url.remove(0, 7);

        int primeiraBarra = url.indexOf('/');
        String autoridade;

        if (primeiraBarra >= 0)
        {
            autoridade = url.substring(0, primeiraBarra);
            endereco.caminhoBase = url.substring(primeiraBarra);
        }
        else
        {
            autoridade = url;
        }

        int doisPontos = autoridade.lastIndexOf(':');

        if (doisPontos >= 0)
        {
            endereco.host = autoridade.substring(0, doisPontos);

            long portaLida = autoridade.substring(doisPontos + 1).toInt();

            if (portaLida < 1 || portaLida > 65535)
                return endereco;

            endereco.porta = static_cast<uint16_t>(portaLida);
        }
        else
        {
            endereco.host = autoridade;
        }

        while (endereco.caminhoBase.endsWith("/"))
            endereco.caminhoBase.remove(
                endereco.caminhoBase.length() - 1
            );

        if (endereco.host.length() == 0)
            return endereco;

        endereco.valido = true;
        return endereco;
    }

    int FazerRequisicao(
        Client& cliente,
        const EnderecoAPI& endereco,
        const String& caminho,
        const String& payload)
    {
        HttpClient http(cliente, endereco.host.c_str(), endereco.porta);

        http.beginRequest();
        http.post(caminho.c_str());

        http.sendHeader("Content-Type", "application/json");
        http.sendHeader("X-API-Key", FLUXID_DEVICE_API_KEY);
        http.sendHeader("Content-Length", payload.length());

        http.beginBody();
        http.print(payload);
        http.endRequest();

        Serial.println("POST enviado; aguardando resposta da API...");
        Serial.print("Destino: http://");
        Serial.print(endereco.host);
        Serial.print(":");
        Serial.print(endereco.porta);
        Serial.println(caminho);
        Serial.print("Tamanho do payload: ");
        Serial.println(payload.length());

        int codigo = http.responseStatusCode();

        // Consome o corpo da resposta antes de encerrar a conexão.
        String resposta = http.responseBody();

        Serial.print("HTTP status: ");
        Serial.println(codigo);

        if (resposta.length() > 0)
        {
            Serial.print("Resposta da API: ");
            Serial.println(resposta);
        }

        http.stop();
        return codigo;
    }
}

int EnviarDadosTelemetria(const String& payload)
{
    const uint8_t MAX_TENTATIVAS = 1;

    if (payload.isEmpty())
    {
        Serial.println("Envio cancelado: telemetria indisponivel.");
        return -1;
    }

    EnderecoAPI endereco = InterpretarURL();

    if (!endereco.valido)
    {
        Serial.println("URL da API invalida. Verifique FLUXID_API_BASE_URL.");
        return -1;
    }

    String caminho = endereco.caminhoBase + "/iot/telemetries";

    for (uint8_t tentativa = 1; tentativa <= MAX_TENTATIVAS; tentativa++)
    {
        Client* cliente = nullptr;
        WiFiClient clienteWiFi;

        // if (WiFi.status() == WL_CONNECTED) // comentado até que o gsm seja responsável por fazer a conexão com internet
        // {
        //     cliente = &clienteWiFi;
        //     Serial.println("Transporte selecionado: Wi-Fi");
        // }
        // else if (GPRSConectado())
        // {
        //     cliente = ObterClienteGSM();
        //     Serial.println("Transporte selecionado: GPRS");
        // }
        if (WiFi.status() == WL_CONNECTED)
        {
            cliente = &clienteWiFi;
            Serial.println("Transporte selecionado: Wi-Fi");
        }
        else
        {
            Serial.println("Envio cancelado: Wi-Fi indisponivel.");
            return -1;
        }

        if (cliente == nullptr)
        {
            Serial.println("Sem Wi-Fi ou GPRS disponivel.");
            return -1;
        }

        Serial.print("Tentativa HTTP: ");
        Serial.println(tentativa);

        int codigo = FazerRequisicao(
            *cliente,
            endereco,
            caminho,
            payload
        );

        if (codigo == 200 || codigo == 201 ||
            codigo == 202 || codigo == 409)
        {
            Serial.println(
                codigo == 409
                    ? "Mensagem duplicada ou ja recebida."
                    : "Telemetria aceita pela API."
            );

            return codigo;
        }

        if (codigo == 400 || codigo == 401 ||
            codigo == 403 || codigo == 404 ||
            codigo == 413)
        {
            Serial.println("Erro permanente: verificar payload, chave ou dispositivo.");
            return codigo;
        }

        if (codigo == 429)
        {
            Serial.println("API limitou as requisicoes (HTTP 429).");
        }
        else if (codigo >= 400 && codigo < 500)
        {
            Serial.println("Erro HTTP de cliente; sem repeticao automatica.");
            return codigo;
        }
        else
        {
            Serial.println("Falha temporaria de comunicacao ou servidor.");
        }

        if (tentativa < MAX_TENTATIVAS)
        {
            dadosMensagem.attempt_count++;

            uint32_t esperaMs =
                (codigo == 429) ? 10000UL :
                (tentativa == 1) ? 10000UL : 30000UL;

            delay(esperaMs);
        }
        else
        {
            Serial.println("Limite de tentativas atingido.");
            return codigo;
        }
    }

    return -1;
}

int EnviarDadosTelemetria()
{
    return EnviarDadosTelemetria(
        MontarJsonTelemetria()
    );
}