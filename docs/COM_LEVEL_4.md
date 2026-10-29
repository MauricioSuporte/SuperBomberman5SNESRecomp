# Battle — COM level 4 / Nível 4 do COM

## Português

Em **Mods → Cheats → Battle — COM level 4**, ative **COM level 4 (Super) in
Battle**. Desativado por padrão. Enquanto o mod estiver ligado, a configuração
do Battle deixa escolher o nível 4 (Super) dos jogadores COM, o mesmo que o
cartucho só libera com a senha **1103**. Nada mais muda no Battle nem no
Normal Game. A ROM em disco e o SRAM não são alterados: com o mod desligado,
o jogo volta aos três níveis de sempre (um save que já tenha recebido a senha
1103 continua com o nível 4, como no cartucho). Pacote 0.1.0, exige host
v2.3.0 ou superior.

Como funciona: a tela de senha compara os quatro dígitos com a tabela
`$C5:6875` (dez senhas, cada uma com uma rotina). A rotina da 1103
(`$C5:68E0`) grava 4 em `$7E6247`, o nível máximo que o script de
configuração do Battle aceita: em `$C6:91F1` ele compara o nível escolhido
`$625B` com `$6247` antes de aumentá-lo. No boot, `$C0:319D` grava 3 e
`$C0:486D` só sobe para 4 quando o bit 4 do byte de SRAM `$3061E8` está
ligado (é o bit que a senha grava). O mod mantém `$6247` em 4 no começo de
cada quadro enquanto estiver ativo. É só WRAM, então save states, rewind e
netplay já levam o valor; nenhum gancho do interpretador nem patch de ROM.

Validação: disassembly da imagem JPN (tabela de senhas, rotinas `$C5:68E0`/
`$C5:6909`, boot `$C0:3190`/`$C0:486D` e o script `$C6:91F1`) e o teste nativo
`com_level4_tests`. Ainda **não** foi validado em runtime: falta confirmar no
jogo que o seletor chega ao nível 4 e que os COM jogam nesse nível.

## English

Enable **COM level 4 (Super) in Battle** under **Mods → Cheats → Battle — COM
level 4**. Disabled by default. While the mod is on, the Battle settings let
you pick COM level 4 (Super), which the cartridge otherwise unlocks only with
password **1103**. Nothing else changes in Battle or Normal Game. Cartridge
files and SRAM stay untouched: with the mod off the game offers the stock
three levels again (a save that received password 1103 keeps level 4, as on
the cartridge). Package 0.1.0 requires host v2.3.0 or newer.

How it works: the password screen compares the four digits against the table
at `$C5:6875` (ten passwords, each with a handler). The 1103 handler
(`$C5:68E0`) stores 4 in `$7E6247`, the highest level the Battle settings
script accepts: `$C6:91F1` compares the chosen level `$625B` with `$6247`
before incrementing it. At boot `$C0:319D` stores 3 and `$C0:486D` raises it
to 4 only when SRAM byte `$3061E8` bit 4 is set (the bit the password saves).
The mod keeps `$6247` at 4 at the start of every frame while enabled. It is
WRAM only, so save states, rewind and netplay already carry it; there is no
interpreter hook and no ROM patch.

Validation: JPN disassembly (password table, handlers `$C5:68E0`/`$C5:6909`,
boot `$C0:3190`/`$C0:486D` and script `$C6:91F1`) and the native
`com_level4_tests`. **Not yet validated at runtime**: still to confirm in game
that the selector reaches level 4 and that COM players use it.

## Password table / Tabela de senhas

Research reference (`$C5:6875`, 4 digits + handler; the handler sets a bit in
`$7E6274`, mirrored to SRAM `$3061E8` through `$7E6273`, and only bit `$10` is
reloaded at boot). Effects in the right column come from the community lists
the maintainer collected, not from runtime checks.

| Password | Handler | Bit | Code effect | Reported effect |
|---|---|---|---|---|
| 4622 | `$68C4` | `$01` | `$6246` = 1 | Battle stage variant 2 |
| 0413 | `$68CA` | `$02` | `$6246` = 2 | Battle stage variant 3 |
| 0926 | `$68D0` | `$04` | `$6246` = 3 | Battle stage variant 4 |
| 2657 | `$6928` | `$08` | message 3 | Normal Game with full power-ups |
| 3879 | `$695E` | `$80` | `$C0:499C`, SFX 4, `$1D24/$1D25` = 1, `$C4:286F` | Boss rush |
| 1103 | `$68E0` | `$10` | `$6247` = 4, saved | COM level 4 (this mod) |
| 0110 | `$6997` | `$20` | SFX 3 | Soft blocks become items in Battle |
| 5656 | `$69A9` | `$40` | `$C5:03A9` passes 8 instead of 3 to `$C5:053E` | 9 lives, 8-minute timer |
| 1999 | `$68F1` | `$80` | message `$AA` | Good ending |
| 0803 | `$68FD` | `$80` | message `$AB` | Bad ending |
