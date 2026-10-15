# Compila e executa os testes no computador (sem a placa).
# Usa o compilador MSVC do Visual Studio; nao instala nada.
#
# Uso:  powershell -ExecutionPolicy Bypass -File test\host\run_tests.ps1
#
# Estes testes cobrem logica isolavel (politica de envio, leitura de
# cabecalhos, LEDs). Nao comprovam GPS, Wi-Fi, LittleFS, API ou GSM reais.

$ErrorActionPreference = 'Stop'

$raiz = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$saida = Join-Path ([System.IO.Path]::GetTempPath()) 'fluxid-host-tests'
New-Item -ItemType Directory -Force $saida | Out-Null

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw 'vswhere.exe nao encontrado: instale o Visual Studio com C++.' }

$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio com ferramentas C++ nao encontrado.' }

$vcvars = Join-Path $vs 'VC\Auxiliary\Build\vcvars64.bat'
$env:PATH = (Split-Path $vswhere) + ';' + $env:PATH

# Cada teste: executavel e fontes do firmware que ele usa.
$testes = @(
    @{ nome = 'test_politica_envio';  fontes = @('src\PoliticaEnvio.cpp') },
    @{ nome = 'test_cabecalhos_http'; fontes = @('src\PoliticaEnvio.cpp') },
    @{ nome = 'test_leds';            fontes = @('src\LED.cpp', 'src\Estado.cpp') }
)

$falhou = $false

foreach ($t in $testes) {
    $arquivos = @("test\host\$($t.nome).cpp") + $t.fontes | ForEach-Object { "`"$(Join-Path $raiz $_)`"" }
    $exe = Join-Path $saida "$($t.nome).exe"

    # mocks primeiro: o Arduino.h simulado substitui o real.
    $cmd = "call `"$vcvars`" >nul && cl /nologo /std:c++17 /EHsc /W4 /WX " +
           "/I`"$raiz\test\host\mocks`" /I`"$raiz\src`" " +
           "/Fo`"$saida\\`" /Fe`"$exe`" $($arquivos -join ' ')"

    Write-Host "== $($t.nome)"
    cmd /c $cmd
    if ($LASTEXITCODE -ne 0) { Write-Host "COMPILACAO FALHOU: $($t.nome)"; $falhou = $true; continue }

    & $exe
    if ($LASTEXITCODE -ne 0) { $falhou = $true }
}

if ($falhou) { Write-Host 'RESULTADO: FALHOU'; exit 1 }
Write-Host 'RESULTADO: TODOS OS TESTES PASSARAM'
