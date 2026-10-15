# Bomber Kings / Reis Bomber do Super Bomberman 4

## Português

Em **Mods**, ative **Play as the Bomber Kings** e coloque a **sua própria ROM do
Super Bomberman 4** (lançamento japonês, SHA-256
`68df561dba2c47a2b743eb5f960530c7a587646d3e448f68f9937305815c9f44`) ao lado da
ROM do Super Bomberman 5 ou numa pasta `roms` junto do executável. O nome do
arquivo não importa (`.sfc` ou `.smc`, com ou sem cabeçalho de 512 bytes): o mod
confere o hash. Os gráficos e as paletas do Super Bomberman 4 são lidos dessa
ROM em tempo de execução; nada dela fica no projeto ou nos pacotes.

A seleção de personagens passa a ter **três fileiras de seis**:

```
 Bomberman  Pretty   ...  (5 originais)   Honey
 (5 originais de baixo / Subordinate / X)  Kotetsu
 Bomberman SB4  Hammer  Jet  Lady  Bazooka  Great
```

As duas fileiras originais sobem e a nova fileira fica onde era a de baixo. O
mod inclui a coluna da Honey e do Kotetsu (mesmo sem o mod deles ativo). Sem a
ROM do Super Bomberman 4 a seleção continua com duas fileiras e uma mensagem no
log diz onde colocar a ROM.

- **Cursor:** direita/esquerda dão a volta na fileira; cima/baixo passam pelas
  três fileiras (de cima vai para a de baixo e vice-versa).
- **Cores:** com **Selection colors**, L/R escolhem entre as cinco paletas que
  cada personagem tem no Super Bomberman 4 (P1 a P5). Sem esse mod, cada
  jogador usa a cor do seu número, como no Super Bomberman 4.
- **Battle Mode** e, com **Choose a character in Normal Game**, **Normal Game**.
- **Na fase:** eles jogam como o Bomberman do Super Bomberman 5 (mesmas regras,
  hitbox e itens) com os sprites do Super Bomberman 4: andar e parar nas quatro
  direções, andar e parar com a bomba erguida (Power Glove), chute/arremesso,
  pulo, montado no Louie (o Louie mantém as cores originais), comemoração,
  derrota, atordoado, Time Up, placar; retrato na seleção e rosto no HUD.

### Habilidades especiais (B + direção)

Segure **B** e aperte uma **direção** (ou aperte B enquanto segura uma direção):

| Personagem | Habilidade (versão SB5) | Depois de usar |
|---|---|---|
| Honey | tiro em linha reta (até 7 blocos) | debuff |
| Kotetsu | espadada no bloco da frente | debuff |
| Hammer Bomber | martelada no bloco da frente | debuff |
| Jet Bomber | investida a jato até bater (velocidade ×3, invencível) | debuff |
| Lady Bomber | disco em linha reta (até 12 blocos) | recarga de 2 s |
| Bazooka Bomber | jato de fogo em linha reta até um bloco (0,4 s) | recarga de 5 s |
| Great Bomber | fica parado e invencível por 3 s | debuff |
| Bomberman (SB4) | nenhuma (B normal) | — |

- **Debuff** (como no Super Bomberman 4, medido no jogo: Hammer, Jet e Great;
  também Honey e Kotetsu): cerca de **10,7 s** com **metade da velocidade** e a
  habilidade bloqueada.
- **B sozinho** continua com a função original (detonar a bomba-controle). Com
  a habilidade pronta, o jogo espera alguns quadros por uma direção antes de
  detonar.
- **Montado no Louie**, B + direção não usa a habilidade (a habilidade do Louie
  continua no Y).
- **Dano:** usa as regras do próprio cartucho (um ponto de vida e invencibilidade
  temporária nos inimigos e chefes; derrota nos jogadores). No **Normal Game**
  atinge só inimigos e chefes, **nunca** outros jogadores. No **Battle** atinge
  todos os outros jogadores no Free-for-All e só o **time adversário** no Team
  Battle.
- Os efeitos (tiro, disco, golpe, fogo, jato, escudo) são desenhos simples
  feitos no código, não gráficos extraídos.

Limitações: as habilidades seguem o estilo do Super Bomberman 4 adaptado ao
SB5; Hammer e Lady causam dano em vez de derrubar o item do alvo. Jogadores da
CPU não usam habilidades. Lady e Bazooka usam uma recarga (o Super Bomberman 4
não aplica debuff a eles; o tempo de recarga foi escolhido para o SB5). A tela
de campeão do Battle continua com o Bomberman. O mod vem desativado e exige host
v0.10.0 ou superior. Pacote de dados:

```powershell
python tools/package-bomber-kings.py
```

## English

Enable **Play as the Bomber Kings** in **Mods** and place **your own Super
Bomberman 4 ROM** (Japanese release, SHA-256
`68df561dba2c47a2b743eb5f960530c7a587646d3e448f68f9937305815c9f44`) next to the
Super Bomberman 5 ROM or in a `roms` folder next to the executable. The file
name does not matter (`.sfc` or `.smc`, with or without a 512-byte header): the
mod checks the hash. Super Bomberman 4 graphics and palettes are read from that
ROM at runtime; nothing from it is stored in the project or its packages.

The character selection gets **three rows of six**: the two stock rows move up,
Honey/Kotetsu end them, and a new bottom row holds Super Bomberman 4's
Bomberman, Hammer, Jet, Lady, Bazooka and Great Bomber. The mod includes the
Honey/Kotetsu column. Without a Super Bomberman 4 ROM the picker keeps two rows
and the log says where to place the ROM.

- **Cursor:** left/right wrap inside a row; up/down cycle through the three rows.
- **Colors:** with **Selection colors**, L/R choose among each character's five
  Super Bomberman 4 palettes (P1-P5); without it each player uses their own
  number's color, as in Super Bomberman 4.
- **Battle Mode** and, with **Choose a character in Normal Game**, **Normal Game**.
- **In stages** they play as Super Bomberman 5's Bomberman (same rules, hitbox
  and items) drawn with Super Bomberman 4 frames: walking/standing in four
  directions, carrying a bomb, kick/throw, jumping, riding Louie (Louie keeps
  its colors), cheering, defeat, stun, Time Up, score board; picker portrait and
  HUD face.

### Special abilities (B + direction)

Hold **B** and press a **direction** (or press B while holding a direction):
Honey shoots (up to 7 tiles), Kotetsu slashes and Hammer strikes the tile in
front, Jet dashes until stopped (triple speed, invulnerable), Lady throws a disc
(up to 12 tiles), Bazooka fires a flame line up to the next block (0.4 s), and
Great stands invulnerable for 3 s. Super Bomberman 4's Bomberman has none.

- **Debuff** (Super Bomberman 4, measured in game for Hammer, Jet and Great;
  also Honey and Kotetsu): about **10.7 s** at **half speed** with the ability
  blocked. Lady and Bazooka have no debuff in Super Bomberman 4; they get a 2 s
  and a 5 s cooldown here.
- **B alone** keeps its stock meaning (remote-control detonation). With the
  ability ready, the game waits a few frames for a direction before detonating.
- **Riding Louie**, B + direction does not trigger the ability (Louie's own
  skill stays on Y).
- **Damage** follows the cartridge's rules (one hit point plus temporary
  invulnerability for enemies and bosses; defeat for players). **Normal Game**
  hits enemies and bosses only, **never** other players. **Battle** hits every
  other player in Free-for-All and only the **other team** in Team Battle.
- Effects are simple shapes drawn in code, not extracted graphics.

Limits: Hammer and Lady deal damage instead of knocking the target's item off.
CPU players do not use abilities. The Battle champion screen still shows
Bomberman. Disabled by default; requires host v0.10.0 or newer.

## Technical notes

- `src/sb4_rom.c`: finds the image (`$SB5_SB4_ROM`, working directory, `roms/`,
  the folder of the ROM named in `rom.cfg`), checks SHA-256, decodes frames.
  Battle sheets: 64 frames of 32x32, frame `f` at `base + (f/4)*$800 +
  (f%4)*$80`, tile rows `$200` apart (bases: Bomberman `$D2:0000`, Hammer
  `$CB:0000`, Jet `$CC:0000`, Lady `$CB:8000`, Bazooka `$CC:8000`, Great
  `$CD:1800`). Frames 0-11 stand/walk (up, right, down, left), 20-23 ability,
  24-27 defeat, 32-43 carrying, 45-47/53-54 cheer, 48-51 crouch, 52 surprised,
  55 electrocuted, 56-59 riding, 60-63 kick. Five palettes per character `$20`
  apart (file offsets Bomberman `$E000`, Hammer `$1601FA`, Jet `$E1E0`, Lady
  `$1602BA`, Bazooka `$E2A0`, Great `$E360`). Verified with a headless
  emulator: DMA logs of the cartridge's battle uploads and five-player CGRAM.
- Super Bomberman 4 ability input: hold B, then press a direction (`$C2:44FC`);
  debuff counter `$E2,X` starts near 160 and falls 0.25 per frame; speed halves.
- `src/honey_kotetsu_mod.c` is the extra-character core: 18-cell arrays at
  `$7F:F800/$F812`, per-seat choice `$7F:F824` and color `$7F:F829`, tables at
  `$C2:60C0-$612F`, BG rows `$C2:12DC` 12/16/20. Row palettes are reloaded on
  the first line of each row from the raster observer.
- `src/special_abilities.c`: pre-opcode hooks at `$C0:0AF9` (end of the pad
  reader: `$2C/$31/$36/$3B+seat`) and `$C4:3180` (walking steps; the hook also
  sets the Z flag the following `BEQ` reads). State in `$7F:F830-$F86B` (saved
  with WRAM). Team Battle: `$7E625D` bit 0; teams `$6269` bit 7 → `$15A2` bits
  3-4 (`$C4:2A1C`). Player invulnerability `$15A2` bit 2 + `$170A`.
