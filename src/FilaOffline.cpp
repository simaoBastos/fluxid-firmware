
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <FilaOffline.h>
#include <vector>

namespace
{
    const char* DIRETORIO_FILA = "/queue";

    Preferences preferenciasFila;
    bool filaInicializada = false;
    uint32_t sequenciaArquivo = 0;

    String GerarNomeArquivo(uint32_t numero)
    {
        char nome[40];

        snprintf(
            nome,
            sizeof(nome),
            "/queue/msg_%010lu.json",
            static_cast<unsigned long>(numero)
        );

        return String(nome);
    }

    
    void LimparArquivosTemporarios()
    {
        File diretorio = LittleFS.open(DIRETORIO_FILA);

        if (!diretorio || !diretorio.isDirectory())
        {
            if (diretorio)
                diretorio.close();

            return;
        }

        std::vector<String> temporarios;
        File arquivo = diretorio.openNextFile();

        while (arquivo)
        {
            String nome = String(arquivo.name());

            if (!nome.startsWith("/"))
            {
                nome = String(DIRETORIO_FILA) + "/" + nome;
            }

            if (
                !arquivo.isDirectory() &&
                nome.startsWith("/queue/msg_") &&
                nome.endsWith(".tmp")
            )
            {
                temporarios.push_back(nome);
            }

            arquivo.close();
            arquivo = diretorio.openNextFile();
        }

        diretorio.close();

        for (const String& caminho : temporarios)
        {
            if (LittleFS.remove(caminho))
            {
                Serial.print("Temporario removido: ");
                Serial.println(caminho);
            }
            else
            {
                Serial.print("Falha ao remover temporario: ");
                Serial.println(caminho);
            }
        }
    }
}

bool InicializarFilaOffline()
{
    // Nao formata automaticamente a flash.
    if (!LittleFS.begin(false))
    {
        Serial.println(
            "ERRO: nao foi possivel montar o LittleFS."
        );

        filaInicializada = false;
        return false;
    }

    if (!LittleFS.exists(DIRETORIO_FILA))
    {
        if (!LittleFS.mkdir(DIRETORIO_FILA))
        {
            Serial.println(
                "ERRO: nao foi possivel criar /queue."
            );

            filaInicializada = false;
            return false;
        }
    }

    // Remove temporarios abandonados por interrupcoes anteriores.
    LimparArquivosTemporarios();

    if (!preferenciasFila.begin("fluxqueue", false))
    {
        Serial.println(
            "ERRO: nao foi possivel abrir Preferences da fila."
        );

        filaInicializada = false;
        return false;
    }

    

    sequenciaArquivo =
        preferenciasFila.getUInt("sequence", 0);

    filaInicializada = true;

    Serial.print("Fila offline inicializada. Mensagens: ");
    Serial.println(QuantidadeNaFilaOffline());

    return true;
}

bool AdicionarNaFilaOffline(const String& payload)
{
    if (!filaInicializada || payload.isEmpty())
    {
        return false;
    }

    // Reserva um nome que ainda nao esteja em uso.
    String destino;

    do
    {
        sequenciaArquivo++;

        destino = GerarNomeArquivo(sequenciaArquivo);
    }
    while (LittleFS.exists(destino));

    // Persiste a sequencia para sobreviver a reinicializacoes.
    size_t gravados = preferenciasFila.putUInt(
        "sequence",
        sequenciaArquivo
    );

    if (gravados == 0)
    {
        Serial.println(
            "ERRO: nao foi possivel persistir a sequencia da fila."
        );
        return false;
    }

    String temporario = destino + ".tmp";

    File arquivo = LittleFS.open(temporario, "w");

    if (!arquivo)
    {
        Serial.println(
            "ERRO: nao foi possivel criar arquivo da fila."
        );

        return false;
    }

    size_t escritos = arquivo.print(payload);

    arquivo.flush();
    arquivo.close();

    if (escritos != payload.length())
    {
        LittleFS.remove(temporario);

        Serial.println(
            "ERRO: escrita incompleta na fila offline."
        );

        return false;
    }

    // O arquivo so fica visivel como mensagem apos a escrita.
    if (!LittleFS.rename(temporario, destino))
    {
        LittleFS.remove(temporario);

        Serial.println(
            "ERRO: nao foi possivel finalizar arquivo da fila."
        );

        return false;
    }

    Serial.print("Telemetria armazenada: ");
    Serial.println(destino);

    return true;
}

bool ObterProximaDaFilaOffline(String& caminho, String& payload)
{
    caminho = "";
    payload = "";

    if (!filaInicializada)
    {
        return false;
    }

    File diretorio = LittleFS.open(DIRETORIO_FILA);

    if (!diretorio || !diretorio.isDirectory())
    {
        return false;
    }

    String caminhoMaisAntigo = "";

    File arquivo = diretorio.openNextFile();

    while (arquivo)
    {
        String nome = String(arquivo.name());

        if (!nome.startsWith("/"))
        {
            nome = String(DIRETORIO_FILA) + "/" + nome;
        }

        bool arquivoJSON =
            !arquivo.isDirectory() &&
            nome.endsWith(".json") &&
            nome.indexOf("msg_") >= 0;

        if (
            arquivoJSON &&
            (
                caminhoMaisAntigo.isEmpty() ||
                nome.compareTo(caminhoMaisAntigo) < 0
            )
        )
        {
            caminhoMaisAntigo = nome;
        }

        arquivo.close();
        arquivo = diretorio.openNextFile();
    }

    diretorio.close();

    if (caminhoMaisAntigo.isEmpty())
    {
        return false;
    }

    File mensagem = LittleFS.open(
        caminhoMaisAntigo,
        "r"
    );

    if (!mensagem)
    {
        return false;
    }

    payload = mensagem.readString();

    mensagem.close();

    
    if (payload.isEmpty())
    {
        Serial.println(
            "AVISO: arquivo vazio encontrado na fila. Removendo."
        );

        if (LittleFS.remove(caminhoMaisAntigo))
        {
            Serial.println("Arquivo vazio removido.");
        }
        else
        {
            Serial.println("ERRO: nao foi possivel remover arquivo vazio.");
        }

        return false;
    }


    caminho = caminhoMaisAntigo;

    return true;
}

bool RemoverDaFilaOffline(const String& caminho)
{
    if (!filaInicializada || caminho.isEmpty())
    {
        return false;
    }

    if (!LittleFS.exists(caminho))
    {
        return true;
    }

    bool removido = LittleFS.remove(caminho);

    if (removido)
    {
        Serial.print("Telemetria removida da fila: ");
        Serial.println(caminho);
    }

    return removido;
}

uint16_t QuantidadeNaFilaOffline()
{
    if (!filaInicializada)
    {
        return 0;
    }

    File diretorio = LittleFS.open(DIRETORIO_FILA);

    if (!diretorio || !diretorio.isDirectory())
    {
        return 0;
    }

    uint16_t quantidade = 0;

    File arquivo = diretorio.openNextFile();

    while (arquivo)
    {
        String nome = String(arquivo.name());

        if (!nome.startsWith("/"))
        {
            nome = String(DIRETORIO_FILA) + "/" + nome;
        }

        if (
            !arquivo.isDirectory() &&
            nome.endsWith(".json") &&
            nome.indexOf("msg_") >= 0
        )
        {
            quantidade++;
        }

        arquivo.close();
        arquivo = diretorio.openNextFile();
    }

    diretorio.close();

    return quantidade;
}


bool LimparFilaOffline()
{
    if (!filaInicializada)
    {
        Serial.println("ERRO: fila nao inicializada.");
        return false;
    }

    File diretorio = LittleFS.open(DIRETORIO_FILA);

    if (!diretorio || !diretorio.isDirectory())
    {
        Serial.println("ERRO: diretorio da fila indisponivel.");
        return false;
    }

    std::vector<String> arquivos;
    File arquivo = diretorio.openNextFile();

    while (arquivo)
    {
        String nome = String(arquivo.name());

        if (!nome.startsWith("/"))
        {
            nome = String(DIRETORIO_FILA) + "/" + nome;
        }

        if (!arquivo.isDirectory() &&
            nome.startsWith("/queue/msg_"))
        {
            arquivos.push_back(nome);
        }

        arquivo.close();
        arquivo = diretorio.openNextFile();
    }

    diretorio.close();

    uint16_t removidos = 0;

    for (const String& caminho : arquivos)
    {
        if (LittleFS.remove(caminho))
        {
            removidos++;
        }
        else
        {
            Serial.print("Falha ao remover: ");
            Serial.println(caminho);
        }
    }

    Serial.print("Limpeza concluida. Arquivos removidos: ");
    Serial.println(removidos);

    Serial.print("Mensagens restantes: ");
    Serial.println(QuantidadeNaFilaOffline());

    return QuantidadeNaFilaOffline() == 0;
}
