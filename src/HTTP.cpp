
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoHttpClient.h>

#include <HTTP.h>
#include <CabecalhosHTTP.h>
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

    // Esperas da requisicao (etapa 3). A conexao TCP usa o padrao do
    // WiFiClient (3 s); se FLUXID_API_BASE_URL usar nome de host, o DNS
    // pode somar ate cerca de 15 s antes disso.
    const uint32_t TIMEOUT_RESPOSTA_MS = 10000UL;   // ate a linha de status
    const uint32_t TIMEOUT_CABECALHOS_MS = 3000UL;  // demais cabecalhos
    const uint32_t TIMEOUT_CORPO_MS = 1000UL;       // por byte do corpo

    // Adapta o HttpClient ao leitor de CabecalhosHTTP.h.
    struct LeitorHttpClient
    {
        HttpClient& http;

        bool endOfHeadersReached() { return http.endOfHeadersReached(); }
        int available() { return http.available(); }
        int readHeader() { return http.readHeader(); }
        bool conectado() { return http.connected(); }
        void esperar() { delay(1); }
    };

    uint32_t Agora()
    {
        return millis();
    }

    ResultadoEnvio FazerRequisicao(
        Client& cliente,
        const EnderecoAPI& endereco,
        const char* metodo,
        const String& caminho,
        const String& payload)
    {
        ResultadoEnvio resultado;
        resultado.retry_after_s = 0;
        resultado.retry_after_invalido = false;
        resultado.requisicao_realizada = true;

        HttpClient http(cliente, endereco.host.c_str(), endereco.porta);
        http.setHttpResponseTimeout(TIMEOUT_RESPOSTA_MS);
        http.setTimeout(TIMEOUT_CORPO_MS);

        http.beginRequest();
        http.startRequest(caminho.c_str(), metodo);

        if (payload.length() > 0)
            http.sendHeader("Content-Type", "application/json");

        http.sendHeader("X-API-Key", FLUXID_DEVICE_API_KEY);

        if (payload.length() > 0)
        {
            http.sendHeader("Content-Length", payload.length());
            http.beginBody();
            http.print(payload);
        }

        http.endRequest();

        Serial.print(metodo);
        Serial.println(" enviado; aguardando resposta da API...");
        Serial.print("Destino: http://");
        Serial.print(endereco.host);
        Serial.print(":");
        Serial.print(endereco.porta);
        Serial.println(caminho);

        if (payload.length() > 0)
        {
            Serial.print("Tamanho do payload: ");
            Serial.println(payload.length());
        }

        resultado.codigo = http.responseStatusCode();

        String resposta;

        if (resultado.codigo > 0)
        {
            // Os cabecalhos precisam ser lidos antes do corpo.
            LeitorHttpClient leitor{http};
            ResultadoCabecalhos cabecalhos =
                LerCabecalhosHTTP(leitor, Agora, TIMEOUT_CABECALHOS_MS);

            resultado.retry_after_s = cabecalhos.retry_after_s;
            resultado.retry_after_invalido = cabecalhos.retry_after_invalido;

            // Consome o corpo da resposta antes de encerrar a conexão.
            if (cabecalhos.completos)
                resposta = http.responseBody();
            else
                Serial.println(
                    "AVISO: cabecalhos HTTP incompletos "
                    "(tempo esgotado ou conexao encerrada); corpo ignorado."
                );
        }

        Serial.print("HTTP status: ");
        Serial.println(resultado.codigo);

        if (resposta.length() > 0)
        {
            Serial.print("Resposta da API: ");
            Serial.println(resposta);
        }

        if (resultado.retry_after_s > 0)
        {
            Serial.print("Retry-After (s): ");
            Serial.println(resultado.retry_after_s);
        }
        else if (resultado.retry_after_invalido)
        {
            Serial.println("AVISO: Retry-After presente, mas ilegivel.");
        }

        http.stop();
        return resultado;
    }

    ResultadoEnvio ResultadoNaoRealizado()
    {
        ResultadoEnvio resultado;
        resultado.codigo = -1;
        resultado.retry_after_s = 0;
        resultado.retry_after_invalido = false;
        resultado.requisicao_realizada = false;
        resultado.acao = ENVIO_REPETIR;
        return resultado;
    }

    // Valida URL e Wi-Fi e faz a requisicao; nada e transmitido se falhar.
    ResultadoEnvio Requisitar(
        const char* metodo,
        const String& sufixo,
        const String& payload)
    {
        EnderecoAPI endereco = InterpretarURL();

        if (!endereco.valido)
        {
            Serial.println("URL da API invalida. Verifique FLUXID_API_BASE_URL.");
            return ResultadoNaoRealizado();
        }

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
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("Envio cancelado: Wi-Fi indisponivel.");
            return ResultadoNaoRealizado();
        }

        Serial.println("Transporte selecionado: Wi-Fi");

        ResultadoEnvio resultado = FazerRequisicao(
            clienteWiFi,
            endereco,
            metodo,
            endereco.caminhoBase + sufixo,
            payload
        );

        resultado.acao = ClassificarRespostaHTTP(resultado.codigo);
        return resultado;
    }
}

ResultadoEnvio EnviarDadosTelemetria(const String& payload)
{
    if (payload.isEmpty())
    {
        Serial.println("Envio cancelado: telemetria indisponivel.");
        return ResultadoNaoRealizado();
    }

    ResultadoEnvio resultado = Requisitar(
        HTTP_METHOD_POST,
        "/iot/telemetries",
        payload
    );

    if (!resultado.requisicao_realizada)
        return resultado;

    switch (resultado.acao)
    {
        case ENVIO_CONFIRMADO:
            Serial.println(
                resultado.codigo == 409
                    ? "Mensagem duplicada ou ja recebida."
                    : "Telemetria aceita pela API."
            );
            break;

        case ENVIO_REJEITADO:
            Serial.println("Erro permanente: corpo da mensagem invalido.");
            break;

        case ENVIO_PAUSAR:
            Serial.println(
                resultado.codigo == 404
                    ? "Dispositivo nao cadastrado na API (HTTP 404)."
                    : resultado.codigo == 403
                        ? "Chave de outro dispositivo ou dispositivo desativado (HTTP 403)."
                        : "Chave ausente ou invalida (HTTP 401)."
            );
            break;

        case ENVIO_REPETIR:
            Serial.println(
                resultado.codigo == 429
                    ? "API limitou as requisicoes (HTTP 429)."
                    : "Falha temporaria de comunicacao ou servidor."
            );
            break;
    }

    return resultado;
}

ResultadoEnvio VerificarAutorizacaoAPI()
{
    // Rota documentada do proprio dispositivo, autenticada com a mesma
    // X-API-Key. So lista comandos pendentes; nao confirma nenhum.
    Serial.println("Verificando chave e cadastro na API...");

    return Requisitar(
        HTTP_METHOD_GET,
        String("/iot/commands/") + FLUXID_DEVICE_ID,
        String()
    );
}
