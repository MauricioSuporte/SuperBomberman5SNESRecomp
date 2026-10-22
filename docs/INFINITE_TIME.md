# Normal Game — infinite time / Tempo infinito

## Português

Compile as fontes atuais. Em **Mods → Cheats → Normal Game — infinite time**,
ative **Unlimited time in Normal Game** e inicie o jogo. Desativado por padrão.
Todas as fases Normal usam o cronômetro nativo `-:--`, sem morte por tempo.
Battle e menus mantêm o comportamento original. Bombas e inimigos ainda matam.
O pacote 0.1.0 vem incluído na v0.4.3 e exige host v0.4.3 ou superior.
A ROM em disco e o SRAM não são alterados.

Hooks em `$C5:0400`, `$C5:0460` e `$C5:0BEF` aplicam os valores nativos
99 segundos/9 minutos antes do desenho inicial, expiração e atualização do HUD.
Não altera flags de boss. Encadeia o hook anterior e restaura no reset.
As rotinas precisam continuar interpretadas. A WRAM já contém todo o estado;
não há nova extensão de save state nem estado adicional de gameplay.

Testes nativos cobrem ativação, contextos, encadeamento e reset. Rotas
Windows/LLE na Zone 1-1 exercitam 1/2 jogadores, PT-BR e personagens/cores,
com tentativa de forçar 0:01. Campanha completa, bosses reais, rewind e
controles físicos não foram qualificados por esta mudança.

## English

Build current source and enable **Unlimited time in Normal Game** under
**Mods → Cheats → Normal Game — infinite time**. Disabled by default.
Normal stages use native `-:--` with no timeout deaths. Battle and menus
retain their behavior; enemies and bombs still kill players. Package 0.1.0
requires host v0.4.3 or newer and is bundled in the v0.4.3 setup ZIP.
Cartridge files and SRAM remain unchanged.

Interpreter hooks at `$C5:0400`, `$C5:0460`, `$C5:0BEF` apply native unlimited
seconds/minutes (99/9) before initialization drawing, expiration and HUD
updates. Boss flags remain intact. The prior hook is chained and restored
on reset. These routines must remain interpreted. WRAM carries all timer
state; no extra save-state extension or gameplay state is needed.

Native tests cover activation, guards, chaining and reset. Windows/LLE
Zone 1-1 routes exercise 1/2 players, PT-BR and character/color selection,
including a forced 0:01 deadline. Full campaign, real bosses, rewind and
physical controllers remain unqualified by this change.

Since v1.2.0 the mod is listed under **Cheats** (next to infinite lives) instead of
Gameplay. / Desde a v1.2.0 o mod fica em **Cheats** (junto com vidas infinitas).
