# Normal Game — infinite lives / Vidas infinitas

## Português

Em **Mods → Cheats → Normal Game — infinite lives**, ative **Unlimited lives in
Normal Game**. Desativado por padrão. No Normal Game o contador de vidas nunca
diminui: morrer, reiniciar a fase e voltar com Start no modo de cinco jogadores
não gastam vida, então não há Game Over. Battle não muda. A ROM em disco e o
SRAM não são alterados. Pacote 0.1.0, exige host v1.2.0 ou superior.

Como funciona: o cartucho gasta vidas (`$1D23`) em `$C5:0659` (renascer) e na
reinicialização da fase `$C5:3B1C-$3B2C` (um `DEC` por participante, seguido do
teste de Game Over em `$C5:3B2D`); o mod de cinco jogadores cobra os assentos
3–5 no próprio gancho em `$C5:3B1C`. O gancho do interpretador guarda o valor
antes de qualquer cobrança e o devolve em `$C5:3B2D`/`$C5:065C`. Nenhum byte
da ROM muda e não há estado novo de save state (o valor só vive dentro da
mesma instrução). As rotinas precisam continuar interpretadas.

## English

Enable **Unlimited lives in Normal Game** under **Mods → Cheats → Normal Game —
infinite lives**. Disabled by default. Normal Game never spends a life: deaths,
stage retries and five-player Start rejoins keep the counter, so there is no
Game Over. Battle is unchanged; cartridge files and SRAM stay untouched.
Package 0.1.0 requires host v1.2.0 or newer.

The interpreter hook captures `$1D23` at `$C5:3B1C`/`$C5:0659`, before the
co-op hook it wraps charges seats 3–5, and restores it at `$C5:3B2D`/`$C5:065C`,
before the native Game Over sign test. Verified by JPN disassembly of every
`$1D23` writer (`$C4:247A`, `$C4:288E/$289A`, `$C5:0659`, `$C5:0E34`,
`$C5:3B23/$3B2A`, `$C5:433A`) and native tests; runtime evidence is in
`docs/STATUS.md`.
