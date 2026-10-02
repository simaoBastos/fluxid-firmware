#include <JSON.h>
#include <GPS.h>

String MontarJson() // estrutura provisória baseada na struct "DadosGPS"
{
    JsonDocument doc;

    doc["satelites"] = dadosGPS.satelites;
    doc["latitude"] = round(dadosGPS.latitude * 1000000.0) / 1000000.0;
    doc["longitude"] = round(dadosGPS.longitude * 1000000.0) / 1000000.0;
    doc["hdop"] = dadosGPS.hdop;

    String dados;
    serializeJson(doc, dados);
    return dados;
}