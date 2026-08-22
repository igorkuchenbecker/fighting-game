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
docs/DECISOES.md       — log de decisões (data | decisão | motivo | alternativas)
AGENTS.md / CLAUDE.md  — este arquivo (symlink)
assets/                — ainda não existe; só entra na F5+ (spritesheets) e
                         F7 (áudio), sempre com fallback automático se ausente
```

Próxima extração de módulo (por ordem do prompt): `stage`/`game` na F4.

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
**F4a — P2 real (sub-fatia da F4): CONCLUÍDA.**

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

- Módulo `combat.h/.cpp` extraído (ordem `input, fighter, combat, stage, game`).
- `MoveData`/`kMoveTable` (`combat.cpp`): tabela central com o único golpe
  existente (`LightAttack`) — startup/active/recovery/dano/chip/hitstun/
  blockstun/pushback/hitbox, tudo num lugar só.
- `FighterHurtbox`/`FighterHitbox` calculam as caixas por frame a partir
  do estado atual (hurtbox encolhe ao agachar; hitbox espelha com
  `facing_right`). `ResolveCombat(atacante, defensor, input_do_defensor)`
  faz o AABB, e se acertar: dano+hitstun+pushback normal, ou chip+
  blockstun+pushback reduzido se o defensor segurar "pra trás" (checado
  reativamente no instante do hit — sem stance de guarda persistente
  ainda, ver `docs/DECISOES.md`).
- `ApplyHitReaction`/`ApplyBlockReaction` (fighter.cpp) aplicam a reação
  sem violar "único ponto de transição"; `Hitstun`/`Blockstun` da FSM
  ganharam gatilho real pela primeira vez.
- Segundo `Fighter` ("dummy", parado, sem input) adicionado só pra
  existir alvo — `ResolveCombat` já é chamado nos dois sentidos (P1→P2 e
  P2→P1), pronto pra quando a F4 trouxer P2 jogável de verdade. Cores
  distintas por lutador (MAROON/DARKBLUE) por já valer a regra visual
  "dois tons pra P1/P2" desde que há 2 corpos em tela.
- Allow-list de estados com controle direcional (`Idle/WalkForward/
  WalkBackward/Jump`) substituiu a deny-list da F2 — trava movimento em
  qualquer estado novo por padrão (Hitstun/Blockstun agora corretos).
- Verificado com smoke test temporário (2 cenários scriptados — hit limpo
  e hit bloqueado com o dummy encostado na parede pra não fugir do
  hitbox andando, revertido antes do commit): dano 8/hitstun 14 no hit
  limpo, chip 1/blockstun 8 no bloqueado, pushback correto nos dois
  corpos (inclusive com o clamp de parede segurando o defensor). Build
  limpo nos dois presets, ASan sem leak do nosso código.
- (resumo F2/F3, detalhes nos commits `6e0d46d`/`fb6fe44`): módulos
  `input`/`fighter`/`combat` extraídos; `InputBuffer` de 10 frames;  FSM
  completa (14 estados); ataque neutro com 3 fases; tabela central de
  frame data (`kMoveTable`); hit/hurtbox por frame; dano/hitstun/pushback/
  bloqueio/blockstun/chip damage — tudo validado com dummy estático (F3),
  depois com P2 real (F4a).

## Próxima fatia (F4b — Round/Partida)

Extrair módulos `stage` (bounds/render da arena) e `game` (fluxo de
round/match). Sistema de round best-of-3, timer 99s contado dentro do
step de simulação (`kFixedDt` por tick — determinístico, sem ler
relógio), tela de intro "ROUND 1 / FIGHT!", KO screen. É aqui que
`Knockdown`/`Win`/`Lose` da FSM ganham gatilho pela primeira vez (hit que
zera a vida → Knockdown; fim de round → `Win`/`Lose` via nova função
`SetRoundOutcome`, mantendo a regra de único ponto de transição).
`Wakeup` provavelmente continua sem gatilho (só faz sentido com um
knockdown "soft" que não termina o round — isso é F5+, ver decisão a
registrar). Depois disso, **F4c — HUD**: barras de vida, medidor de super
placeholder (enche ao dar/tomar dano, sem golpe pra gastar ainda),
contador de combo com escala de dano, retratos placeholder.
