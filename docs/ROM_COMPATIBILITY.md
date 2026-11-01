# ROM compatibility / Compatibilidade de ROMs

## English

`rom_identity.txt` names the one **supported** dump, and releases are qualified on it. Other dumps can run, unqualified:

1. **Is it this game?** Internal title starting `Hu SUPER BOMBERMAN 5`, HiROM, at least 2 MiB, valid checksum pair. A wrong
   checksum only warns, because patched ROMs have one. Copier headers are removed first.
2. **Does the code match?** A generated build runs only the ROM it was generated from.
3. **Which mods fit?** Each mod has a CRC over the cartridge sites it names. If that CRC differs, the mod does nothing,
   and the log says `[rom] mod <id> is off`. Matching does not prove a mod's feature works on that ROM.

With an unsupported ROM, the Boss Rush online ranking is off, so scores stay comparable, and netplay is refused, because different ROMs would desync.

The launcher's file picker still accepts only the supported dump. Put other ROMs in the game folder or `roms/`, or pass one on
the command line. A ROM named on the command line or in `rom.cfg` wins; otherwise the supported dump wins over others in the folder.

Results, Linux, headless, reaching Zone 1-1 unless noted:

| ROM (SHA-256) | Size, notes | No mods | Mods on | Mods off by content | 5-player menu |
|---|---|---|---|---|---|
| `5251e649` supported, English | 4 MiB | yes | yes | none | yes, PT-BR works |
| `9be18be1` Japanese | 4 MiB | yes | yes | PT-BR | yes |
| `b8ea7591` Japanese retail | 2 MiB | yes | yes | PT-BR | yes, 3 seats spawned |
| `e0cff27b` `[f1]` | 2 MiB, checksum mismatch | yes | yes | PT-BR | yes |
| `7468ed4c` `[t1]` translation | 2 MiB, copier header, checksum mismatch | title and player menu; stage not reached (its intro is longer than the route) | picker reached | PT-BR | not reached |
| `5f8e6894` Gold Cartridge | 2 MiB | yes | yes | 13 of 15 checked (Pause, Infinite time stay) | no |

## Português

`rom_identity.txt` indica a única ROM **suportada**, e as releases são qualificadas nela. Outras ROMs podem rodar, sem qualificação:

1. **É este jogo?** Título interno começando com `Hu SUPER BOMBERMAN 5`, HiROM, pelo menos 2 MiB, par de checksum válido. Um checksum
   errado só gera aviso, porque ROMs modificadas têm esse erro. Cabeçalhos de copiadora são removidos antes.
2. **O código corresponde?** Uma compilação gerada só roda a ROM da qual foi gerada.
3. **Quais mods servem?** Cada mod tem um CRC dos pontos do cartucho que ele usa. Se esse CRC for diferente, o mod não faz nada, e o
   log mostra `[rom] mod <id> is off`. Corresponder não prova que o recurso do mod funciona naquela ROM.

Com uma ROM não suportada, o ranking online do Boss Rush fica desligado, para as pontuações continuarem comparáveis, e o netplay é recusado, porque ROMs diferentes perderiam a sincronia.

O seletor de arquivos do launcher ainda só aceita a ROM suportada. Coloque outras ROMs na pasta do jogo ou em `roms/`, ou informe uma na linha de
comando. Uma ROM na linha de comando ou no `rom.cfg` tem prioridade; fora isso, a ROM suportada vence as outras da pasta.

A tabela de resultados está na seção em inglês.
