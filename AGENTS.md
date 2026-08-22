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

Abre janela 1280×720. P1 (Warrior) = setas + espaço/enter/shift-direito
(leve/médio/pesado) ou gamepad slot 0; P2 (Gunner) = WASD +
ctrl-esquerdo/shift-esquerdo/Q, ou gamepad slot 1 — Gunner solta um
projétil segurando baixo+pesado. Partida best-of-3 com intro/timer/KO
automáticos. Fecha com o botão de fechar da janela (ou Alt+F4).

## Mapa de arquivos

```
CMakeLists.txt        — alvo fighting_game (7 .cpp), FetchContent do raylib (tag 5.5)
CMakePresets.json      — presets debug/release (flags descritas acima)
src/main.cpp           — orquestração fina: janela, loop de timestep fixo,
                         leitura de input dos 2 jogadores, chama UpdateMatch
                         e as funções de desenho (fighter/arena/projéteis/
                         overlay/HUD)
src/input.h/.cpp       — Direction8, InputFrame (dado puro), InputBuffer
                         (circular, 10 frames), ReadInputFrame (único ponto
                         que toca IsKeyDown/gamepad)
src/fighter.h/.cpp     — FighterState (FSM completa), AttackPhase, MoveId,
                         CharacterId, Fighter, StepFighter (transição +
                         física, puro/determinístico), UpdateFacing,
                         ApplyHitReaction/ApplyBlockReaction/
                         ApplyKnockdownReaction/SetRoundOutcome/AddSuperMeter,
                         ClampFighterToArena, ResetFighterForNewRound
src/combat.h/.cpp      — MoveData, tabela central de frame data (kMoveTable,
                         6 golpes), FighterHurtbox/FighterHitbox, ResolveCombat
                         (corpo-a-corpo) e ResolveProjectileHit (projétil),
                         ambos via ApplyMoveOutcome (dano/hitstun/pushback/
                         bloqueio/blockstun/chip/combo/meter compartilhado)
src/projectile.h/.cpp — Projectile (entidade independente do Fighter),
                         SpawnProjectile/StepProjectile/ProjectileHitbox/
                         DrawProjectile
src/stage.h/.cpp       — kScreenWidth/Height, kArenaLeft/Right/kFloorY, DrawArena
src/game.h/.cpp        — RoundPhase, Match (inclui os 2 slots de Projectile),
                         kFixedDt, UpdateMatch (orquestra facing/física/
                         combate/projéteis durante Fighting + fluxo de
                         round), DrawMatchOverlay (intro/timer/KO/resultado),
                         DrawHud (vida/meter/combo/retrato por jogador),
                         DrawProjectiles
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
**F4 — Partida: CONCLUÍDA** (sub-fatias F4a `dd6c5ae`, F4b `cce743b`, F4c abaixo).

- **F4a** (P2 real): `dummy` virou `p2` jogável — P1 setas+espaço/gamepad
  0, P2 WASD+ctrl-esquerdo/gamepad 1 (`ReadInputFrame` combina teclado+
  gamepad por OR). `UpdateFacing(p1, p2)` 1x por tick antes dos
  `StepFighter`: `WalkForward`/`WalkBackward` e o lado do hitbox passam a
  refletir a posição relativa real.
- **F4b** (round/partida): módulos `stage`/`game` extraídos (ordem de
  extração do prompt completa). `Match`/`RoundPhase`
  (`Intro→Fighting→Ko→Intro...` ou `→MatchOver`) em `game.h`; timer de
  99s decrementado por `kFixedDt` dentro do step (determinístico).
  `Knockdown` ganhou gatilho real (KO por nocaute); `Win`/`Lose` fecham
  cada round, reaproveitados no `MatchOver`. `DrawMatchOverlay`: "ROUND
  N"/"FIGHT!"/timer/"K.O."/"TIME UP"/resultado final.
- **F4c** (HUD, esta fatia): `Fighter::combo_hits`/`super_meter` novos.
  `combat.cpp`: hit sem bloqueio escala dano por combo (`ComboDamageScale`
  — 100%/1º hit, −10%/hit, piso 50%) e incrementa `combo_hits` do
  defensor (reseta em `StepFighter` ao SAIR de `Hitstun`, não em
  `ApplyHitReaction` — resetar ali zeraria toda vez, nunca contando 2+);
  hit e bloqueio geram medidor de super pros dois lados
  (`AddSuperMeter`, valores diferentes hit vs. block). `DrawHud`
  (`game.cpp`): barra de vida + medidor de super por jogador (P2
  espelhado/ancorado à direita), texto "N HITS" quando `combo_hits>=2`,
  retrato placeholder (quadrado com a cor do lutador).
- `BlockStanding`/`BlockCrouching`/`Wakeup` seguem sem gatilho (não
  teriam comportamento distinto do que já existe — ver `docs/DECISOES.md`
  em cada fatia). Caminho de `Timeout` do round não foi exercitado
  ponta-a-ponta (99s reais = ~5940 ticks); combo real de 2+ hits também
  não é alcançável em timing de jogo ainda (1 golpe só, não cancelável,
  recovery+startup > o próprio hitstun que causa) — ambos os casos
  raciocinados/testados isoladamente, documentado em `docs/DECISOES.md`.
  Controle físico de teclado/gamepad nunca foi testado com hardware real
  (sem ferramenta de simulação de input no ambiente).
- Build limpo nos dois presets, ASan sem leak do nosso código em toda a
  F4.

**F5a — Moveset básico (sub-fatia da F5): CONCLUÍDA.**

- `MoveId` (4 golpes: `LightStanding`/`MediumStanding`/`HeavyStanding`/
  `CrouchingLight`) movido pra `fighter.h`; `Fighter::current_move` novo,
  decidido por `DetermineMove` (fighter.cpp) no instante em que `Attack`
  é acionado — agachado sempre vira `CrouchingLight` independente do
  botão, em pé respeita leve/médio/pesado.
- `kMoveTable` (combat.cpp) agora tem 4 entradas com frame data distinta
  (pesado mais lento/forte: startup 13/dano 18; leve mais rápido/fraco:
  startup 6/dano 8). `HeavyStanding` dobra de antiaéreo: hitbox alto e
  alongado verticalmente (offset y −170, altura 110), sem golpe/input
  dedicado.
- `Fighter::buttons_held` (bitmask) substitui o antigo bool único — borda
  de subida detectada por bit (`buttons_down & ~buttons_held`), suporta
  os 3 botões de ataque.
- `fighter.cpp` passou a incluir `combat.h` (`AdvanceAttackPhase` lê
  `GetMoveData(fighter.current_move)` pra saber startup/active/recovery
  do golpe atual, já que cada golpe agora tem duração própria).
- Mapa de teclas expandido: P1 leve=Espaço/médio=Enter/pesado=Shift-dir;
  P2 leve=Ctrl-esq/médio=Shift-esq/pesado=Q; gamepad usa os 3 botões de
  face inferior/direito/esquerdo.
- Verificado via `StepFighter` real (não só `ResolveCombat` direto, pra
  provar que `DetermineMove` funciona no caminho de jogo de verdade):
  cada botão (+ agachado) escolhe o `MoveId` certo; golpe antiaéreo
  testado isoladamente contra um defensor reposicionado perto do ápice
  do pulo — conectou com o dano correto (18). Build limpo nos dois
  presets, ASan sem leak (saída limpa forçada com limite de frames
  temporário, já que o loop real só fecha por `WindowShouldClose`).

**F5b — Salto com ataque (sub-fatia da F5): CONCLUÍDA.**

- `ComputeNextState`'s caso `Jump` agora checa `attack_just_pressed`
  antes do check de `is_grounded` (dá pra atacar em qualquer ponto do
  pulo, contanto que ainda esteja no ar).
- `MoveId::JumpingLight` (5º golpe, `kMoveTable` combat.cpp): startup 5/
  active 6/recovery 8/dano 10, hitbox na altura do peito (offset y −70,
  altura 50). Ignora força do botão (só existe 1 golpe aéreo, mesma
  lógica do agachado da F5a). `DetermineMove` checa `Jump` antes de
  `Crouch`.
- Nenhum caso especial pra momentum aéreo (`Attack` zera `velocity.x`
  igual no chão/ar) nem pra pouso durante recovery (resolve sozinho, sem
  código extra) — ver `docs/DECISOES.md`.
- Verificado via `StepFighter` real: pula, ataca logo no início do pulo
  (baixa altitude) e conecta com dano correto (10) + Hitstun; também
  confirmado que atacar perto do ápice do pulo erra um alvo no chão —
  física correta (hitbox fica alto demais), não bug. Pouso após o ataque
  resolve normalmente de volta a Idle. Build limpo nos dois presets,
  ASan sem leak (saída limpa forçada com limite de frames temporário).

**F5c — 2º personagem + projétil (sub-fatia da F5): CONCLUÍDA.**

- Módulo `projectile.h/.cpp` novo: `Projectile` (posição/velocidade/
  tempo de vida próprios, entidade independente do `Fighter`),
  `SpawnProjectile`/`StepProjectile`/`ProjectileHitbox`/`DrawProjectile`.
  Cada jogador tem no máx. 1 ativo por vez (`Match::p1_projectile`/
  `p2_projectile`, slots fixos, sem alocação dinâmica).
- `CharacterId` (`Warrior`/`Gunner`) novo em `Fighter`; P1=Warrior,
  P2=Gunner (`main.cpp`). Kit quase idêntico — único diferenciador
  mecânico: Gunner troca o agachado padrão por `MoveId::Projectile`
  quando segura pesado agachado (`DetermineMove`); os outros 4 golpes
  são compartilhados entre os dois (frame data por personagem de verdade
  é trabalho da F5e — ver `docs/DECISOES.md`).
  `ResetFighterForNewRound` corrigido pra preservar `character` através
  do reset de round (senão Gunner virava Warrior no round 2).
- `MoveId::Projectile` (6º golpe, `kMoveTable`): startup 8/recovery 20/
  dano 12; seu campo `hitbox` na tabela é `{0,0,0,0}` (não usado —
  `ResolveCombat` pula golpes `Projectile`, quem resolve é
  `ResolveProjectileHit` contra o `Projectile` spawnado).
  `game.cpp`/`MaybeSpawnProjectile` detecta a borda Startup→Active
  (attack_phase antes/depois do `StepFighter` daquele tick) e spawna.
- `ApplyMoveOutcome` (combat.cpp) extraído: dano/combo/reação/medidor
  compartilhados entre `ResolveCombat` e `ResolveProjectileHit` — só o
  pushback difere (projétil só empurra o alvo, sem corpo pra recuar).
- Verificado via `UpdateMatch` real (fase `Fighting` forçada, pulando a
  intro): Warrior com baixo+pesado continua no agachado normal; Gunner
  com baixo+pesado dispara o projétil; projétil nasce na posição/direção
  certas, atravessa a arena e acerta com dano correto (12), desativando
  ao conectar. Descoberto (não bug): segurar baixo+ataque no MESMO tick
  a partir de Idle dispara o golpe em pé, não o agachado — precisa
  primeiro entrar em Crouch antes de atacar, como na maioria dos
  fighting games reais (documentado em `docs/DECISOES.md`). Build limpo
  nos dois presets, ASan sem leak.

## Próxima fatia (F5d — Super de verdade)

Ativa com `super_meter` (já existe e enche desde a F4c, mas nada ainda
consome). Precisa: gatilho de input (provavelmente segurar 2 botões de
ataque juntos, já que não há motion input tipo quarter-circle
implementado — decisão a registrar), consumir o medidor (100→0 ao
ativar), invulnerabilidade inicial (`Fighter` precisa de algo tipo
`bool is_invulnerable` ou um novo `AttackPhase`/estado que `ResolveCombat`
e `ResolveProjectileHit` ignorem), e um `MoveId::Super` na tabela com
dano alto. Depois: **F5e** — balanceamento de verdade da frame data (os
valores de hoje são placeholder consistente, não testado por jogo real;
inclui dar ao Warrior algo que o distinga mais do Gunner além do
projétil, se fizer sentido nessa passada).
