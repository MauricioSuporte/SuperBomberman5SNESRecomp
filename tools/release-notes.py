"""Generate initial prototype release notes; keep output outside tracked source."""
from pathlib import Path
import argparse
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path)
parser.add_argument('--version', help='Version for correcting an existing release without changing VERSION.')
args = parser.parse_args()

version = args.version or (Path(__file__).resolve().parents[1] / 'VERSION').read_text().strip()
notes = f'''# Super Bomberman 5 v{version} — Windows prototype

Unofficial noncommercial prototype. **Partial recompilation:** most game logic still uses SNESRecomp's interpreter. The opening, title, menus, Zone 1 map and first-stage entry were verified locally over a scripted 3,628-frame route. Windows display/audio initialization and launcher smoke tests passed. Full-game accuracy, audio fidelity, battle mode and P2–P5 are not qualified; netplay and widescreen enhancements are not implemented.

Download the Windows x64 ZIP and extract the entire archive into a writable folder. Start the root SuperBomberman5SNESRecomp.exe, let the launcher obtain official build tools, select your own legally obtained ROM and use Generate & rebuild. The native analyzer is bundled; release users do not need Rust. The first tool download needs internet. Requires Windows 10+ x64 and several GB of disk space.

The release contains **a setup host and source, not the game**: no ROM, ROM-derived C, ROM-derived game executable, extracted sprites/music, saves or gameplay captures. Runtime licensed launcher assets and dependency notices are preserved. The playable build is generated locally under build/.

Supported image: header region JPN, CRC32 c25ab56a, SHA-256 5251e6491872dc51879b630a34d76a92e29ea0c39026c4f8773c1fe006d047c4. The developer filename contains USA, but hashes determine compatibility. No other revision is qualified.

Reset/NMI remain interpreted after AOT promotion produced continuation/dispatch errors. This is a baseline for improving native coverage, not a completed port. See README.md, docs/STATUS.md, CONTRIBUTING.md and THIRD_PARTY_NOTICES.md. Verify the ZIP using SHA256SUMS.

## Português

Protótipo não oficial e não comercial para Windows x64. **Recompilação parcial:** a maior parte da lógica continua no interpretador. Foram verificados abertura, título, menus, mapa da Zone 1 e entrada na primeira fase numa rota de 3.628 frames, além de janela, inicialização do áudio e launcher. Jogo completo, fidelidade sonora, batalha e P2–P5 não foram validados; não há netplay nem melhorias de widescreen.

Extraia todo o ZIP, abra SuperBomberman5SNESRecomp.exe na raiz, permita baixar as ferramentas oficiais, selecione sua própria ROM e use Generate & rebuild. O analisador já está incluído; o usuário da release não precisa instalar Rust. A primeira instalação exige internet, Windows 10+ x64 e alguns GB livres.

O pacote contém **instalador e fontes, não o jogo**. Não inclui ROM, fontes/executável derivados da ROM, sprites, música, saves ou capturas. O jogo é gerado e compilado localmente em build/. Confira SHA256SUMS, os hashes acima e as limitações em docs/STATUS.md.
'''
if tuple(int(n) for n in version.split('.')) >= (0,2,0):
    notes += '''\n## PT-BR menu mod / Mod de menus em português\n\nThe optional PT-BR preview translates mapped title, player/file selection, options, sound test and battle setup/rules labels with authored glyphs. Enable it in Mods. It is not a full-game translation; unmapped screens and gameplay/ending messages remain original. Presentation-only, disabled by default; see docs/TRANSLATION_PTBR.md.\n\nA prévia PT-BR traduz rótulos de título, jogadores/arquivos, opções, teste de som e menus/regras de batalha com uma fonte própria. Ative em Mods. Não é tradução completa: telas não mapeadas e mensagens de fases/finais permanecem originais. Mod de apresentação, desabilitado por padrão; veja docs/TRANSLATION_PTBR.md.\n'''
if args.output:
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(notes, encoding='utf-8', newline='\n')
else:
    sys.stdout.reconfigure(encoding='utf-8')
    print(notes, end='')
