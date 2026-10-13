# Português brasileiro — primeiro mod (prévia de menus)

Pacote `superbomberman5.ptbr`, versão `0.1.0`, feature `menus_ptbr`, plugin nativo confiável `superbomberman5.ptbr.menus`. Desabilitado por padrão. Só aceita a ROM de referência de `rom_identity.txt`.

## Como testar

Compile o código atual, que prepara a versão 0.2.0 (`./tools/build-windows.ps1`), e abra `play.cmd`. No launcher, entre em **Mods**, encontre **Português brasileiro — menus (prévia)**, ative **Menus em português brasileiro** e selecione Play. Desative a feature para voltar aos textos originais. A release antiga do jogo **v0.1.0 não inclui este plugin**; instalar o pacote nela não adiciona código nativo.

Para gerar o arquivo instalável, sem ler a ROM:

```powershell
python tools/generate-ptbr.py --mod-out dist/superbomberman5-ptbr-menus-0.1.0.snesmod
```

O `.snesmod` contém manifesto, textos/metadados autorais e licença, sem DLL, executável, ROM ou gráficos extraídos. A implementação nativa precisa estar compilada no jogo. A versão 0.1.0 está incluída no catálogo preloaded deste checkout, portanto não é necessário importar o arquivo para testar localmente.

## Escopo

| Original | Tradução |
|---|---|
| PUSH START BUTTON | PRESSIONE START |
| NORMAL GAME | JOGO NORMAL |
| BATTLE GAME | BATALHA |
| OPTIONS | OPÇÕES |
| New Game | NOVO JOGO |
| Continue | CONTINUAR |
| Erase File | APAGAR |

Primeira versão parcial. Banners gráficos, submenu de quantidade de jogadores, seleção de arquivos, opções internas, batalha, mensagens de fases e finais ainda precisam de mapeamento/tradução. Logo, retratos, áudio e mapas permanecem originais.

## Implementação e dados

`translations/ptbr/labels.json` contém texto e coordenadas/identificadores de tiles como metadados funcionais. Não contém bitmaps do jogo. `tools/generate-ptbr.py` valida limites e gera um header dentro da pasta de build. `src/ptbr_mod.c` contém uma fonte própria de 5×7, suporte a cedilha/til e o plugin de apresentação.

Durante o renderer LLE, o mod identifica rótulos por camada, posição e IDs esperados, desenha novos tiles temporários e restaura toda a VRAM antes de o driver reaplicar escritas pendentes do jogo. Não altera arquivo de ROM nem WRAM; o renderer HLE ainda não está qualificado para essa integração. Não há fallback que declare o jogo inteiro traduzido.

As capturas de validação ficam em `.bootstrap/ptbr-shots/` e nunca são publicadas. O teste `ptbr_render_restore` verifica mod ativo, guarda de superfície, restauração integral da VRAM e mod desativado. Execute `./build-lle/ptbr_render_tests.exe` após compilar. Se CTest estiver instalado, também pode usar `ctest --test-dir build-lle --output-on-failure`; o pacote portátil atual não inclui CTest.
