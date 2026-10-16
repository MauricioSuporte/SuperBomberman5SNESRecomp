# Bomber Kings / Reis Bomber do Super Bomberman 4

## Português

Em **Mods**, ative **Play as the Bomber Kings**. Logo abaixo aparece o item
**Super Bomberman 4 ROM**: clique para escolher a **sua própria ROM do Super
Bomberman 4** (lançamento japonês, SHA-256
`68df561dba2c47a2b743eb5f960530c7a587646d3e448f68f9937305815c9f44`), do mesmo
jeito que outros recomps pedem uma segunda ROM para alguns mods. O launcher
confere o hash (com ou sem cabeçalho de 512 bytes) e mostra **ROM verified**;
enquanto ela não for escolhida, o Play pede para resolver o mod. O caminho fica
salvo nas configurações de mods. Os gráficos e as paletas do Super Bomberman 4
são lidos dessa ROM em tempo de execução; nada dela fica no projeto ou nos
pacotes, e o arquivo nunca é copiado nem alterado.

Com o Subordinate Bomber e o mod de cores ativos ao mesmo tempo, as seis
figuras da terceira fileira continuam aparecendo: quando faltam entradas OAM
livres, elas reaproveitam as de sprites nativos que terminam acima da fileira,
trocadas na linha em que a fileira começa e restauradas no fim do quadro.

A seleção de personagens passa a ter **três fileiras de seis**:

```
 Bomberman  Pretty   ...  (5 originais)   Honey
 (5 originais de baixo / Subordinate / X)  Kotetsu
 Bomberman SB4  Hammer  Jet  Lady  Bazooka  Great
```

As duas fileiras originais sobem e a nova fileira fica onde era a de baixo. O
mod inclui a coluna da Honey e do Kotetsu (mesmo sem o mod deles ativo).

- **Cursor:** direita/esquerda dão a volta na fileira; cima/baixo passam pelas
  três fileiras (de cima vai para a de baixo e vice-versa).
- **Cores:** com **Selection colors**, L/R escolhem entre as cinco paletas que
  cada personagem tem no Super Bomberman 4 (P1 a P5). Sem esse mod, cada
  jogador usa a cor do seu número, como no Super Bomberman 4.
- **Battle Mode** e, com **Choose a character in Normal Game**, **Normal Game**.
- **Na fase:** eles jogam como o Bomberman do Super Bomberman 5 (mesmas regras,
  hitbox e itens) com os sprites do Super Bomberman 4: andar e parar nas quatro
  direções, andar e parar com a bomba erguida (Power Glove), chute/arremesso,
  pulo, montado no Louie (com as poses de montaria do Super Bomberman 4: ao
  subir e montado; o Louie mantém as cores originais), comemoração, derrota, atordoado, Time Up, Bad
  Bomber (a prancha do Super Bomberman 4 em cada lado da arena, também ao
  arremessar), placar; retrato na seleção e rosto no HUD.
- **Cores na fase:** cada jogador usa a sua própria paleta, trocada só nas
  linhas da tela onde ele está desenhado; o Louie, os ovos e o rosto no HUD do
  mesmo jogador continuam com as cores normais. Quando outro sprite da mesma
  paleta está nas mesmas linhas (montado, ovo chocando), o personagem usa uma
  paleta livre.

### Habilidades especiais (B + direção)

Segure **B** e aperte uma **direção** (ou aperte B enquanto segura uma direção).
As habilidades dos Reis Bomber são as do Super Bomberman 4, com os mesmos
tempos, poses e gráficos (lidos da sua ROM do SB4); as da Honey e do Kotetsu
seguem os ataques deles na luta contra eles no Normal Game:

| Personagem | Habilidade | Depois de usar |
|---|---|---|
| Honey | fica parada na animação de tiro da luta contra ela (0,9 s) e dispara um projétil que segue sozinho até um bloco e mata quem acertar | debuff |
| Kotetsu | estocada de espada da luta contra ele (1 s), atingindo o bloco da frente | debuff |
| Hammer Bomber | fica parado girando a maça em volta de si (~2 s); quem ela toca fica atordoado e perde um item | — |
| Jet Bomber | preparação de 0,8 s, depois voa a 4 px por quadro até bater deixando um rastro de fogo; ao bater fica parado no obstáculo na pose de voo por 24 quadros (como no SB4) e só então volta a andar; pisca invencível do começo até cerca de 1 s depois de bater; durante a habilidade o corpo dele também acerta inimigos e jogadores que toca e ele é imune ao contato com inimigos | debuff |
| Lady Bomber | gira a arma, dispara o laser (sai aos 0,8 s, 2 px por quadro, segue sozinho até um bloco ou alvo), gira a arma de novo e volta a andar (1,8 s); o alvo fica atordoado e perde dois itens | — |
| Bazooka Bomber | fica parado 1,6 s mirando (gira nas quatro direções) e dispara um raio de fogo para a frente como a chama de uma bomba: queima o primeiro soft block, para em blocos fixos e detona bombas no caminho | debuff |
| Great Bomber | fica parado, piscando e invencível, por 2,7 s | debuff |
| Bomberman (SB4) | nenhuma (B normal) | — |

- **Debuff** (como no Super Bomberman 4: Bazooka, Jet e Great; também Honey e
  Kotetsu): cerca de **10,7 s** contados a partir do uso, com **metade da
  velocidade** e a habilidade bloqueada. Hammer e Lady não têm debuff.
- **Sons:** os efeitos sonoros das habilidades são os do Super Bomberman 4
  (raio do Bazooka e rastro do Jet, maça do Hammer, laser da Lady, Great),
  tocados pelo próprio driver de som do SB4, que roda separado do SB5 a partir
  da sua ROM do SB4.
- Enquanto a habilidade dura o personagem não anda nem solta bombas (o Jet
  anda sozinho durante o voo).
- **B sozinho** continua com a função original (detonar a bomba-controle). Com
  a habilidade pronta, o jogo espera alguns quadros por uma direção antes de
  detonar.
- **Montado no Louie**, B + direção não usa a habilidade (a habilidade do Louie
  continua no Y).
- **Dano:** o raio do Bazooka, o rastro e o corpo do Jet, o tiro da Honey e o corte do
  Kotetsu são letais como uma explosão (nos inimigos e chefes, um ponto de vida
  com a invencibilidade temporária do próprio cartucho). A maça e o laser
  atordoam e fazem perder itens com a reação do próprio SB5 (o item sai voando).
  No **Normal Game** atingem só inimigos e chefes, **nunca** outros jogadores.
  No **Battle** atingem todos os outros jogadores no Free-for-All e só o **time
  adversário** no Team Battle.
- **Retratos:** na seleção, os jogadores com um Rei Bomber mostram o retrato do
  próprio Super Bomberman 4 (o da seleção do Battle), na cor escolhida.

Limitações: Jogadores da CPU não usam habilidades. A Honey e o Kotetsu não
têm som de habilidade; a velocidade do projétil da Honey (4 px por quadro) foi
escolhida, porque a IA da chefe não disparou nas capturas. A tela de campeão do Battle
continua com o Bomberman. Os ataques da Honey e do Kotetsu seguem a descrição
da luta (a IA dos chefes não foi reproduzida nas capturas). O mod vem desativado
e exige host v0.12.0 ou superior. Pacote de dados:

```powershell
python tools/package-bomber-kings.py
```

## English

Enable **Play as the Bomber Kings** in **Mods**. Its **Super Bomberman 4
ROM** item appears below it: select **your own Super Bomberman 4 ROM**
(Japanese release, SHA-256
`68df561dba2c47a2b743eb5f960530c7a587646d3e448f68f9937305815c9f44`), the same
way other recomps ask for a second ROM for some mods. The launcher checks the
hash (with or without a 512-byte header) and shows **ROM verified**; until a ROM
is selected, Play asks you to resolve the mod. The path is saved with the mod
settings. Super Bomberman 4 graphics and palettes are read from that ROM at
runtime; nothing from it is stored in the project or its packages, and the file
is never copied or modified.

With Subordinate Bomber and the color mod on at the same time, all six
third-row figures stay visible: when idle OAM entries run out they reuse the
entries of native sprites that end above the row, switched on the row's first
line and restored at the end of the frame.

The character selection gets **three rows of six**: the two stock rows move up,
Honey/Kotetsu end them, and a new bottom row holds Super Bomberman 4's
Bomberman, Hammer, Jet, Lady, Bazooka and Great Bomber. The mod includes the
Honey/Kotetsu column.

- **Cursor:** left/right wrap inside a row; up/down cycle through the three rows.
- **Colors:** with **Selection colors**, L/R choose among each character's five
  Super Bomberman 4 palettes (P1-P5); without it each player uses their own
  number's color, as in Super Bomberman 4.
- **Battle Mode** and, with **Choose a character in Normal Game**, **Normal Game**.
- **In stages** they play as Super Bomberman 5's Bomberman (same rules, hitbox
  and items) drawn with Super Bomberman 4 frames: walking/standing in four
  directions, carrying a bomb, kick/throw, jumping, riding Louie (Super
  Bomberman 4's mount poses, jumping on and riding; Louie keeps its colors), cheering, defeat, stun,
  Time Up, Bad Bomber (Super Bomberman 4's board on each side of the arena,
  throwing too), score board; picker portrait and HUD face.
- **Colors in stages:** each player's own palette is recolored only on the
  screen lines where the character is drawn, so that player's Louie, eggs and
  HUD face keep their colors. When another sprite of that palette shares those
  lines (riding, an egg hatching), the character takes a free palette.

### Special abilities (B + direction)

Hold **B** and press a **direction** (or press B while holding a direction).
The Bomber Kings' abilities are Super Bomberman 4's, with its timings, poses
and graphics (read from your SB4 ROM); Honey's and Kotetsu's follow their
attacks in their Normal Game boss fight:

- **Honey**: stands in her boss shooting animation (0.9 s) and fires a bullet
  that flies on its own up to a block and kills whoever it hits. **Kotetsu**:
  his boss sword thrust (1 s) on the tile in front.
- **Hammer**: stands still swinging his mace around himself (about 2 s); a
  player it touches is stunned and loses an item.
- **Jet**: 0.8 s wind-up, then flies 4 px per frame until stopped, leaving a
  fire trail; after the crash he stays at the obstacle in the flying pose for
  24 frames (as in SB4) before walking again. He blinks
  invulnerable from the start until about 1 s after the crash. During the
  ability his body also hits enemies and players it touches, and enemy
  contact cannot kill him.
- **Lady**: turns her gun, fires the laser (leaves at 0.8 s, 2 px per frame and
  flies on its own up to a block or a target), turns the gun again and walks
  again (1.8 s); the target is stunned and loses two items.
- **Bazooka**: stands 1.6 s aiming (the pose turns through the four
  directions), then fires a fire beam forward like a bomb's flame: it burns
  the first soft block, stops at hard blocks and sets off bombs in its way.
- **Great**: stands still, blinking and invulnerable, for 2.7 s.
- Super Bomberman 4's Bomberman has none.

- **Debuff** (Super Bomberman 4: Bazooka, Jet and Great; also Honey and
  Kotetsu): about **10.7 s** counted from the trigger, at **half speed** with
  the ability blocked. Hammer and Lady have none.
- **Sounds:** the abilities' sound effects are Super Bomberman 4's (Bazooka's
  beam and Jet's trail, Hammer's mace, Lady's laser, Great), played by SB4's
  own sound driver running separately from SB5's, from your SB4 ROM.
- During an ability the character neither walks nor drops bombs (Jet flies on
  his own).
- **B alone** keeps its stock meaning (remote-control detonation). With the
  ability ready, the game waits a few frames for a direction before detonating.
- **Riding Louie**, B + direction does not trigger the ability (Louie's own
  skill stays on Y).
- **Damage:** Bazooka's beam, Jet's trail and body, Honey's shot and Kotetsu's cut are
  lethal like an explosion (enemies and bosses lose one hit point with the
  cartridge's temporary invulnerability). The mace and the laser stun and take
  items with Super Bomberman 5's own reaction (the item flies off). **Normal
  Game** hits enemies and bosses only, **never** other players. **Battle** hits
  every other player in Free-for-All and only the **other team** in Team
  Battle.
- **Portraits:** in the picker, seats with a Bomber King show the character's
  own Super Bomberman 4 Battle picker portrait in the chosen color.

Limits: CPU players do not use abilities. Honey and Kotetsu have no ability
sound; Honey's bullet speed (4 px per frame) was chosen, as the boss AI did not
fire in captures. The Battle champion screen still
shows Bomberman. Honey's and Kotetsu's attacks follow the boss fight as
described (the bosses' AI was not reproduced in captures). Disabled by default;
requires host v0.12.0 or newer (first included in the v0.12.0 setup packs).

## Technical notes

- Full five-seat picker (cursor boxes, "L+R" hints, Subordinate Bomber's
  figure): too few idle OAM entries for the third row's six figures, which
  used to vanish. The scanline renderer evaluates OAM per line, so a figure
  takes the entry of a native sprite that ends at least two lines above the
  row, rewritten when the raster observer reaches the row (`late_places`)
  and restored on line 224 with the other overlay writes.

- Manifest `[[external_rom]]` `sb4-rom` (`normalized_sha256`, `required = true`):
  the launcher asks for the file and verifies it; the plugin reads the path with
  `snes_mod_runtime_resource_path_c`. `src/sb4_rom.c` checks SHA-256 again and
  decodes frames (development fallback without a launcher selection:
  `$SB5_SB4_ROM`, working directory, `roms/`, the folder of the ROM in `rom.cfg`).
  Battle sheets: 64 frames of 32x32, frame `f` at `base + (f/4)*$800 +
  (f%4)*$80`, tile rows `$200` apart (bases: Bomberman `$D2:0000`, Hammer
  `$CC:8000`, Jet `$CC:0000`, Lady `$CB:8000`, Bazooka `$CB:0000`, Great
  `$CD:1800`). Frames 0-11 stand/walk (up, right, down, left), 20-23 ability,
  24-27 defeat, 32-43 carrying, 45-47/53-54 cheer, 48-51 crouch, 52 surprised,
  55 electrocuted, 56-59 riding, 60-63 kick. Five palettes per character `$20`
  apart (file offsets Bomberman `$E000`, Hammer `$E2A0`, Jet `$E1E0`, Lady
  `$1602BA`, Bazooka `$1601FA`, Great `$E360`). Verified with a headless
  emulator: DMA logs of the cartridge's battle uploads and five-player CGRAM.
- Super Bomberman 4 ability input: hold B, then press a direction (`$C2:44FC`);
  debuff counter `$E2,X` is 160 on the trigger frame and falls 0.25 per frame;
  speed halves.
- Riding: Super Bomberman 4 shows frames 20-23 while jumping onto a mount
  (state `$1E`) and 16-19 while riding (from the cartridge's uploads with
  Lady and, with `$0522` set to 5, Great on a mount). Bad Bomber: each character's
  extra sheet (Bomberman `$C5:5AED`, Hammer `$C6:8000`, Jet `$C6:4000`, Lady
  `$C6:0000`, Bazooka `$C5:9AED`, Great `$C6:C000`), facing right +`$3000`,
  down +`$3100`, left +`$3800`, up +`$2900`, throwing +`$80`; Super Bomberman
  5's cart is anims `$42-$45`, throwing `$3B-$3E`, with `$15A2` bit 7.
- Sound: SB4 effects are sent by `$C0:071D` (port 0, bit 7 toggles, numbers
  7-bit): `$39` explosion (Bazooka frame 67, Jet trail every 8 frames), `$4E`
  mace (frames 28-92 every 16), `$5A` laser (frame 47), `$5F` Great (trigger).
  `src/sb4_sound.c` runs the driver in a private APU (the runner's SPC700/DSP):
  streams `$31`, `$2E`, `$2F` from the table at `$C0:0190` uploaded with the
  cartridge's handshake (`$C0:029B/$02F0/$02BD`), then renders each effect to
  a PCM clip for the host mixer (`snes_mod_audio_*`); a repeat cuts the last
  one, like the driver's channel.
- Honey: object set `$0D` attack anims `$23-$26` (5, 11, 16, 3, 4, 16
  frames; muzzle flash at 32) and bullet anims `$27-$2A`; Kotetsu `$30-$33`
  (5, 8, 48). Bazooka: soft blocks burn with `$C5:441D` (A = cell, the
  routine Kotetsu's boss AI uses); bombs are in the list at `$0E2A` (next
  `$0EA4`, cell `$0E7C`, fuse `$0E40`); `$1052` bits `$80` hard, `$40` bomb,
  `$20` soft, `$10` item. Jet stops when his position along the flight stops
  changing (sliding round corners does not count); SB4 releases him 24 frames
  after the crash (frame 96 → 120).
- Jet invulnerability is the native one (`$15A2` bit 2 + `$170A`), extended in
  steps of 4 so its blink (low bits of `$170A`, `$C4:2E42`) keeps running.
  Enemy contact ignores that bit, so the cartridge's 22 kill stores
  (`STA $15A2,Y` at `$C1:19A6`, `$C3:1197`... and `STA $15A2,X` at `$C0:2DAA`,
  `$C4:42E5`..., found by scanning for `LDA/ORA #$40` before them) get a
  pre-opcode hook that drops bit 6 for a seat in Jet's ability; each site is
  hooked only if the image holds the expected `99/9D A2 15` bytes.
- Ability poses (32x32, same layout as the sheets): Bazooka aiming
  `$CA:98B0 + dir*$80`; Jet `$CA:D0B0 + dir*$800 + k*$80` (k 0-3); Lady
  `$CA:A0B0 + k*$800 + dir*$80` (k 0-5); Hammer `$CD:0900`, `$CD:0980`,
  `$CD:1000` (SB4 directions up, right, down, left).
- Fire (explosion BG tiles): streams with headers at `$D1:D019` and
  `$D1:E01C` (loader `$C4:09A5`: zero marker, end marker, 32-byte tiles),
  palette at file `$17E11C`; block `code` = tiles code, +1, +16, +17, phase in
  the high nibble (2-A), kind in the low one (`$C` centre, `$A`/`$8` middle,
  `$6` right end, `$0` top end). Effect OBJ tiles: stream at `$C8:FD36`
  (loader `$C1:874D`), OBJ tile `$80` mace, `$82`/`$84` laser, drawn with the
  player's palette. Picker portraits: raw tiles `$CD:E587` (Bomberman,
  Bazooka, Lady; rows `$200` apart) and `$CD:ED87` (Great, Jet, Hammer; rows
  `$184` apart), palettes at file `$16783C` (Bomberman), `$16799C` (Bazooka),
  `$167A3C` (Lady), `$167ADC` (Jet), `$167B7C` (Hammer), `$167C1C` (Great).
  Timelines measured frame by frame in a headless emulator.
- `src/honey_kotetsu_mod.c` is the extra-character core: 18-cell arrays at
  `$7F:F800/$F812`, per-seat choice `$7F:F824` and color `$7F:F829`, tables at
  `$C2:60C0-$612F`, BG rows `$C2:12DC` 12/16/20. Row palettes are reloaded on
  the first line of each row from the raster observer.
- `src/special_abilities.c`: pre-opcode hooks at `$C0:0AF9` (end of the pad
  reader: `$2C/$31/$36/$3B+seat`) and `$C4:3180` (walking steps; the hook also
  sets the Z flag the following `BEQ` reads). State in `$7F:F830-$F8D9` (saved
  with WRAM). Stun and item loss: `$15BA` bit 5 (the cartridge's own reaction
  `$C4:4618` → `$C4:4631`: dazed 50 frames, `JSR $37CD` with A = 0 throws the
  first item); Lady's second item calls `$C4:37CD` from the pad hook through
  pushed return frames (`$C0:0AF9` and the `RTL` at `$C4:36D1`). Jet's flight
  holds the invulnerability timer, so the overlay draws him at his object. Team Battle: `$7E625D` bit 0; teams `$6269` bit 7 → `$15A2` bits
  3-4 (`$C4:2A1C`). Player invulnerability `$15A2` bit 2 + `$170A`.
