#include <HTTP.h>
#include <JSON.h>
#include <Arduino.h>
#include <HTTPClient.h>

void EnviarDados()
{
    /*
     * HTTP provisoriamente desativado enquanto o contrato
     * ESP32 <-> WebService não estiver definido.
     */

    // HTTPClient Http;
    // String url = "";
    // Http.begin(url);

    // Http.addHeader("Content-Type","application/json");

    String dados = MontarJson();

    Serial.print("Dados para envio: ");
    Serial.println(dados);
    
    // int codigo = Http.POST(dados);

    // switch (codigo)
    // {
    // case 200:
    //     Serial.print("Requisição realizada com sucesso! - ");
    //     Serial.println(codigo);
    //     break;
    
    // default:
    //     Serial.print("Código inválido - ");
    //     Serial.println(codigo);
    //     break;
    // }

    // Http.end();
}