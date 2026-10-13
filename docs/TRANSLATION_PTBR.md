# Português brasileiro — primeiro mod (prévia de menus)

Pacote `superbomberman5.ptbr`, versão `0.1.0`, feature `menus_ptbr`, plugin nativo confiável `superbomberman5.ptbr.menus`. Desabilitado por padrão. Só aceita a ROM de referência de `rom_identity.txt`.

## Como testar

Use a release v0.2.0 e conclua Generate & rebuild com sua ROM. Para compilar este checkout, use `./tools/build-windows.ps1` e abra `play.cmd`. No launcher, entre em **Mods**, encontre **Português brasileiro — menus (prévia)**, ative **Menus em português brasileiro** e selecione Play. Desative a feature para voltar aos textos originais. A release antiga do jogo **v0.1.0 não inclui este plugin**; instalar o pacote nela não adiciona código nativo.

Para gerar o arquivo instalável, sem ler a ROM:

```powershell
python tools/generate-ptbr.py --mod-out dist/superbomberman5-ptbr-menus-0.1.0.snesmod
```

O `.snesmod` contém manifesto, textos/metadados autorais e licença, sem DLL, executável, ROM ou gráficos extraídos. A implementação nativa precisa estar compilada no jogo. A versão 0.1.0 está incluída no catálogo preloaded deste checkout, portanto não é necessário importar o arquivo para testar localmente.

## Escopo

A prévia foi ampliada para título, quantidade de jogadores, modo normal, arquivos, opções, teste de som e menus de batalha, incluindo regras e o cabeçalho de seleção de fases. Os cabeçalhos azuis e o título da tela de senha também são substituídos. Bombermania é um nome próprio e continua como BOMBERMANIA.

| Original | Tradução |
|---|---|
| PUSH START BUTTON | PRESSIONE START |
| NORMAL GAME | JOGO NORMAL |
| BATTLE GAME | BATALHA |
| OPTIONS | OPÇÕES |
| New Game | NOVO JOGO |
| Continue | CONTINUAR |
| Erase File | APAGAR |
| 1 PLAYER GAME | 1 JOGADOR |
| 2 PLAYER GAME | 2 JOGADORES |
| NORMAL MODE | MODO NORMAL |
| SELECT A FILE | ESCOLHA ARQUIVO |
| File 1 | ARQUIVO 1 |
| File 2 | ARQUIVO 2 |
| File 3 | ARQUIVO 3 |
| Password | SENHA |
| Sound | SOM |
| Sound Test | TESTE DE SOM |
| Stereo | ESTÉREO |
| BATTLE MODE | MODO BATALHA |
| Battle Royale | BATALHA LIVRE |
| Bombermania | BOMBERMANIA |
| Config Battle Mode | CONFIGURAÇÃO |
| BATTLE TYPE | TIPO DE BATALHA |
| Free-for-All | TODOS CONTRA TODOS |
| Team Battle | POR EQUIPES |
| 1 Player | JOGADOR 1 |
| Human | HUMANO |
| 2 Player | JOGADOR 2 |
| 3 Player | JOGADOR 3 |
| 4 Player | JOGADOR 4 |
| 5 Player | JOGADOR 5 |
| PLAYER SELECT | JOGADORES |
| SOUND TEST | TESTE DE SOM |
| Music | MÚSICA |
| SFX | EFEITOS |
| Narrator | NARRADOR |
| CHARACTER SELECT | ESCOLHA O PERSONAGEM |
| PASSWORD | SENHA |
| RULE SETTINGS | REGRAS |
| CPU Level | NÍVEL CPU |
| Matches | RODADAS |
| Time | TEMPO |
| Sudden Death | MORTE SÚBITA |
| Bad Bomber | REVANCHE |
| Bowling Bomber | BOLICHE |
| Low | BAIXO |
| Random | ALEATÓRIO |
| On | SIM |
| Mono | MONO |
| Super | SUPER |
| Normal | NORMAL |
| Strong | FORTE |
| Off | NÃO |
| STAGE SELECT | ESCOLHA A FASE |

Ainda é uma tradução parcial. Textos de telas não mapeadas, confirmações de arquivo, nomes/descritivos das arenas (Stage/The Classic etc.), telas ainda não verificadas de equipes/Bombermania, mensagens de fases e finais podem permanecer originais. Tradução de menus não comprova a precisão de partidas de batalha ou multiplayer. Logo, retratos, áudio e mapas permanecem originais.

## Implementação e dados

A prévia contém 62 rótulos de background e um cabeçalho de sprites. Os rótulos de uma mesma tela são identificados antes de qualquer substituição, evitando que a alteração do primeiro texto invalide os demais. IDs reutilizados por telas de jogadores e regras recebem guardas adicionais de contexto. Variantes SIM/NÃO, NORMAL/FORTE e MONO/ESTÉREO compartilham slots temporários.

`translations/ptbr/labels.json` contém texto e coordenadas/identificadores de tiles como metadados funcionais. Não contém bitmaps do jogo. `tools/generate-ptbr.py` valida limites e gera um header dentro da pasta de build. `src/ptbr_mod.c` contém um alfabeto próprio, algarismos, acentos portugueses e estilos de título, texto de menu, banner e senha, além do plugin de apresentação.

Durante o renderer LLE, o mod identifica rótulos por camada, posição e IDs esperados, desenha novos tiles temporários e restaura toda a VRAM antes de o driver reaplicar escritas pendentes do jogo. A fonte do título respeita os quatro pixels de margem superior do original, evitando que a janela HDMA usada para piscar Start corte JOGO NORMAL. Slots de tiles são estáveis por tela; textos que ocupam antigos espaços vazios mantêm a paleta do início do rótulo. As áreas reservadas ficam separadas dos tiles de texto originais. O cabeçalho de fases usa uma faixa de nove sprites de 16×16: o mod verifica posições, tamanho, atributos e tiles, substitui apenas os pixels exclusivos dessa faixa e restaura a VRAM. A OAM não é alterada. Não altera arquivo de ROM nem WRAM; o renderer HLE ainda não está qualificado para essa integração. Não há fallback que declare o jogo inteiro traduzido.

As capturas de validação ficam em `.bootstrap/ptbr-shots/` e nunca são publicadas. O teste `ptbr_render_restore` verifica mod ativo/desativado, guardas, restauração integral da VRAM, margem do título, slots estáveis e paleta de textos maiores. Capturas consecutivas cobrem Start visível/apagado e as telas traduzidas; arquivos de captura ficam apenas em `.bootstrap/`. Execute `./build-lle/ptbr_render_tests.exe` após compilar. Se CTest estiver instalado, também pode usar `ctest --test-dir build-lle --output-on-failure`; o pacote portátil atual não inclui CTest.
