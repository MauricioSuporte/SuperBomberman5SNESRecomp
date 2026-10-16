# README images / Imagens do README

`five-player-showcase.png` (1028 × 900) combines four runtime frames, at the
maintainer's explicit request, to show the optional features on the
repository page. The top-left frame was captured on Windows/LLE for v0.12.1
(corrected 1 PLAYER / 2-5 PLAYERS). The top-right and bottom-right frames were captured from this project's
own Linux LLE build of v0.13.0 on 2026-10-16. Only the bottom-left panel
was replaced for v0.13.1, using a Windows LLE runtime frame:

| Position | Scene | Mods |
|---|---|---|
| Top left | Title, five-player Normal Game menu | Normal co-op |
| Top right | Normal Game character selection with seven columns and three rows: Emperor Terrorin, Honey, Jet, Lady and Great Bomber chosen by the five seats, Super Bomberman 4 portraits, colors changed with L/R | Co-op, character selection, colors, Super Bomberman 5 Extra Characters, Bomber Kings |
| Bottom left | Zone 1-1: green Subordinate Bomber facing down on native Black Louie (type 6), Honey's shot pose, black Jet flying with fire trail, red Bazooka's beam, Terrorin's blue barrier; P1/P2 selected-character HUD faces | Same plus Normal HUD faces, Louie improvements and infinite Normal time |
| Bottom right | Battle Mode stage 1: Kotetsu's sword thrust, Bazooka's fire beam, Hammer's mace, Honey's shot and Terrorin inside his barrier | Extra Characters, Bomber Kings |

The Super Bomberman 4 characters are drawn at runtime from the maintainer's
own Super Bomberman 4 ROM; no data from it is stored in the repository. Each
frame is a 256 × 224 rendering scaled 2× with nearest-neighbour sampling.
Seats 3-5 were driven by a local, uncommitted script extension and player
positions in the Zone 1-1 frame were set with scripted pokes so the abilities
have room. The pictured game artwork belongs to its respective rights holders;
it is displayed as a screenshot of the running software and is not a source
asset, is not covered by the project's software license and is never packaged
in release downloads. No extracted graphics are tracked.

`five-player-showcase.png` reúne quatro quadros reais da execução, a pedido do
mantenedor. O quadro superior esquerdo (menu de cinco jogadores) vem da
v0.12.1; os outros três foram capturados do build Linux LLE da v0.13.0:
seleção de sete colunas com Terrorin, Honey, Jet, Lady e Great; ZONE1-1 com a
barreira do Terrorin, o voo do Jet, a Honey, a Lady e o Great usando as
habilidades; e Battle com a espada do Kotetsu, o raio do Bazooka, a maça do
Hammer, a Honey e o Terrorin na barreira. Os personagens do SB4 vêm da ROM do
SB4 do mantenedor durante a execução. A arte do jogo pertence aos respectivos
detentores; a imagem não entra nos downloads de release e não é coberta pela
licença do software.

`portal-destinations.png` (512 × 448) is the gameplay screenshot supplied by
the maintainer on 2026-10-15 for the v0.8.0 portal-mod explanation. It shows
ZONE1-1 with Z1-2/Z1-5/Z1-7 hints and is authorized for README/docs/releases.
It contains game artwork owned by its respective rights holders; it is not
covered by the software license and is not an extracted sprite/tile asset.
The packagers continue to omit documentation screenshots from setup payloads.

`portal-destinations.png` é a captura fornecida pelo mantenedor para explicar
o mod de portais na v0.8.0. Sua publicação no README foi expressamente
solicitada em 2026-10-15. A arte continua pertencendo aos respectivos titulares.

For the v0.13.1 bottom-left frame, five seats were selected through the real Normal picker. The cartridge egg pickup allocated the native Black Louie for P1; WRAM checks confirmed rider bit 7, mount type 6, Subordinate index 9/color 4 and down direction 2. The staged shot uses scripted positions, open ability corridors, protection for the other seats and explicit native facing/animation state for the mount; it is a presentation showcase rather than an unmodified playthrough. Honey and Terrorin have their single cartridge palettes; Jet uses SB4 color 1, Bazooka color 2 and Subordinate uses green. The other three panels are pixel-identical to v0.13.0. No generated artwork or extracted sprites were composited.

Na v0.13.1 somente o quadro inferior esquerdo foi substituído por captura Windows LLE: Subordinado verde de frente, no Black Louie nativo, Honey, Jet preto voando, Bazooka vermelho com raio e barreira do Terrorin, além da HUD nova de P1/P2. A seleção e a coleta do ovo executam as rotinas reais; posições, corredores, proteção e pose nativa da montaria foram ajustados por script para compor a cena. Honey e Terrorin têm uma paleta nativa cada. Os outros três quadros foram preservados pixel a pixel. É uma cena preparada de apresentação, não uma demonstração de jogatina sem intervenções.
