# AGENTS.md — Fighting Game 2D (C++20 / raylib)

Fonte de verdade para qualquer agente (ou humano) retomando este projeto.
Mantido atualizado ao fim de cada fase. `CLAUDE.md` é symlink para este arquivo.

## Como compilar

```sh
# Debug — ASan + UBSan, -Wall -Wextra -Wpedantic -Werror
cmake --preset debug
cmake --build --preset debug

# Release — -O2, mesmos warnings tratados como erro
cmake --preset release
cmake --build --preset release
```

Build limpo é lei: qualquer warning quebra a build (`-Werror`). O primeiro
`cmake --preset` de cada um baixa e compila o raylib do zero via
`FetchContent` (demora alguns minutos); builds seguintes são incrementais.

## Como rodar

```sh
./build/release/fighting_game   # ou ./build/debug/fighting_game
```

Abre janela 1280×720. Fecha com o botão de fechar da janela (ou Alt+F4).
Cada tick de simulação (60Hz) imprime `tick` no stdout.

## Mapa de arquivos

```
CMakeLists.txt        — alvo fighting_game (main+input+fighter), FetchContent
                         do raylib (tag 5.5)
CMakePresets.json      — presets debug/release (flags descritas acima)
src/main.cpp           — orquestração: janela, loop de timestep fixo, leitura
                         de input (via input.h) e render (DrawArena/DrawFighter)
src/input.h/.cpp       — Direction8, InputFrame (dado puro), InputBuffer
                         (circular, 10 frames), ReadInputFrame (único ponto
                         que toca IsKeyDown)
src/fighter.h/.cpp     — FighterState (FSM completa), AttackPhase, Fighter,
                         StepFighter (transição + física, puro/determinístico),
                         ApplyHitReaction/ApplyBlockReaction, ClampFighterToArena
src/combat.h/.cpp      — MoveId, MoveData, tabela central de frame data
                         (kMoveTable), FighterHurtbox/FighterHitbox, ResolveCombat
src/stage.h/.cpp       — kScreenWidth/Height, kArenaLeft/Right/kFloorY, DrawArena
src/game.h/.cpp        — RoundPhase, Match, kFixedDt, UpdateMatch (orquestra
                         facing/física/combate durante Fighting + fluxo de
                         round), DrawMatchOverlay (textos de intro/timer/KO)
docs/DECISOES.md       — log de decisões (data | decisão | motivo | alternativas)
AGENTS.md / CLAUDE.md  — este arquivo (symlink)
assets/                — ainda não existe; só entra na F5+ (spritesheets) e
                         F7 (áudio), sempre com fallback automático se ausente
```

Ordem de extração do prompt (`input, fighter, combat, stage, game`) completa.

## Convenções (resumo — ver o prompt original para o texto completo)

- RAII absoluto, proibido `new`/`delete` cru.
- Identificadores em inglês; comentários em português só quando a intenção
  não é óbvia (invariantes, mágica de frame data).
- structs + funções livres + enums > hierarquias de classes.
- Timestep de simulação fixo em 60Hz (acumulador), desacoplado do render.
  Nada de física ligada a delta-time variável.
- Determinismo: RNG com seed explícita (nunca `rand()`/`time()`), nenhuma
  leitura de relógio/disco/input dentro do step de simulação, iteração em
  ordem estável (sem `unordered_map` na sim).
- Uma fatia = compila limpo nos dois presets + roda sem leak + 1 commit.
- Decisões autônomas vão para `docs/DECISOES.md`, uma linha por item.

## Estado atual

**F0 — Fundação: CONCLUÍDA** (commit `52b2785`).
**F1 — Lutador vivo: CONCLUÍDA** (commit `42a8202`).
**F2 — FSM + buffer: CONCLUÍDA** (commit `6e0d46d`).
**F3 — Combate: CONCLUÍDA** (commit `fb6fe44`).
**F4a — P2 real (sub-fatia da F4): CONCLUÍDA** (commit `dd6c5ae`).
**F4b — Round/Partida (sub-fatia da F4): CONCLUÍDA.**

- Módulos `stage.h/.cpp` (arena/render) e `game.h/.cpp` (round/match)
  extraídos — ordem de extração do prompt completa.
- `Match`/`RoundPhase` (`Intro→Fighting→Ko→Intro...` ou `→MatchOver`) em
  `game.h`; `UpdateMatch` só roda a simulação (facing/física/combate)
  durante `Fighting` — nas outras fases os lutadores ficam congelados.
- Timer de 99s decrementado por `kFixedDt` (constante) dentro do step de
  sim — determinístico, sem ler relógio.
- `Knockdown` ganhou gatilho real (vida chega a 0 por um hit → `Ko` por
  nocaute); `Win`/`Lose` fecham cada round (reaproveitados também no
  `MatchOver`, sem estados extras). `Wakeup` segue sem gatilho (só faria
  sentido com knockdown "soft" que não termina o round — F5+).
  `BlockStanding`/`BlockCrouching` seguem sem gatilho (ver F4a).
- `DrawMatchOverlay`: "ROUND N"/"FIGHT!" na intro, timer numérico durante
  a luta, "K.O."/"TIME UP" na tela de KO, resultado final no fim da
  partida (best-of-3, `kWinsNeeded=2`).
- Verificado com smoke test temporário (input adaptativo por posição/fase
  em vez de números de frame fixos, vida do P2 forçada baixa pra pular o
  grind de hits reais, revertido antes do commit): 2 rounds completos —
  Intro(120f)→Fighting→KO→Win/Knockdown→placar 1-0→Ko(90f)→reset→Intro
  round 2→Fighting→segundo KO→placar 2-0→`MatchOver` (congela
  corretamente). **Caminho de time-up não exercitado ponta-a-ponta** (99s
  reais = ~5940 ticks, impraticável no smoke test; reusa a mesma
  `EndRound()` já provada, só troca a condição de entrada — ver
  `docs/DECISOES.md`). Build limpo nos dois presets, ASan sem leak.

- F4 foi dividida em sub-fatias (F4a/F4b/F4c) por ser grande demais pra
  uma fatia só — ver `docs/DECISOES.md`.
- `dummy` virou `p2` de verdade: P1 = setas+espaço ou gamepad slot 0; P2 =
  WASD+ctrl-esquerdo ou gamepad slot 1 (`ReadInputFrame` agora lê teclado
  E gamepad, combinados por OR; d-pad ou stick esquerdo com deadzone 0.4).
- `UpdateFacing(p1, p2)` chamado 1x por tick antes dos `StepFighter`:
  `facing_right` passa a refletir a posição relativa real — `WalkForward`/
  `WalkBackward` e o lado do hitbox agora fazem sentido geométrico de
  verdade (antes `facing_right` era fixo `true`).
  `ResolveCombat` já era chamado nos dois sentidos desde a F3, então
  qualquer jogador pode atacar o outro sem mudança nenhuma ali.
- `BlockStanding`/`BlockCrouching` continuam self-loop (ver
  `docs/DECISOES.md` — não teriam comportamento distinto de `Walk*`/
  `Crouch` do jeito que o bloqueio já funciona).
- Verificado com smoke test temporário (P1 e P2 scriptados — P2 aproxima
  e ataca P1, depois P1 pinado na parede bloqueia um segundo ataque de
  P2; revertido antes do commit): facing correto nos dois lados, dano/
  hitstun/chip/blockstun/pushback idênticos aos da F3 mas agora com P2
  como atacante. **Não verificado com tecla/gamepad físico de verdade**
  (sem ferramenta de simulação de input no ambiente) — a leitura
  `IsKeyDown`/gamepad em si (dentro de `ReadInputFrame`) não foi
  exercitada, só a lógica de simulação a jusante dela. Build limpo nos
  dois presets, ASan sem leak do nosso código.

- (resumo F2/F3/F4a, detalhes nos commits `6e0d46d`/`fb6fe44`/`dd6c5ae`):
  módulos `input`/`fighter`/`combat` extraídos; `InputBuffer` de 10
  frames; FSM completa (14 estados); ataque neutro com 3 fases; tabela
  central de frame data (`kMoveTable`); hit/hurtbox por frame; dano/
  hitstun/pushback/bloqueio/blockstun/chip damage; P2 real (teclado+
  gamepad) com facing dinâmico via `UpdateFacing`.

## Próxima fatia (F4c — HUD)

Barras de vida (uma por jogador, provavelmente no topo, ladeando o
timer que `DrawMatchOverlay` já desenha); medidor de super placeholder
que enche ao dar/tomar dano (`Fighter::super_meter`, campo novo — ainda
sem golpe especial pra gastar, isso é F5); contador de combo com escala
de dano (`Fighter::combo_hits`, incrementado em hits consecutivos sem o
defensor voltar a Idle, resetado quando volta — dano do N-ésimo hit
escalado pra baixo, requisito duro "combo counter com escala de dano");
retratos placeholder (podem ser só retângulos com a cor de cada
jogador). Depois da F4c, a F4 está completa e a próxima fase é a F5
(movesets: 2 personagens, leve/médio/pesado, antiaéreo, agachado, salto
com ataque, 1 projétil, 1 super, frame data balanceado de verdade).
