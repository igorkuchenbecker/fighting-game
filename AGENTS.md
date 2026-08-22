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
                         StepFighter (transição + física, puro/determinístico)
docs/DECISOES.md       — log de decisões (data | decisão | motivo | alternativas)
AGENTS.md / CLAUDE.md  — este arquivo (symlink)
assets/                — ainda não existe; só entra na F5+ (spritesheets) e
                         F7 (áudio), sempre com fallback automático se ausente
```

Próxima extração de módulo (por ordem do prompt): `combat` na F3, `stage`/`game` na F4.

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
**F2 — FSM + buffer: CONCLUÍDA.**

- Módulos extraídos (`input.h/.cpp`, `fighter.h/.cpp`) — regra do prompt
  manda extrair na F2 independente do tamanho do arquivo.
- `InputBuffer` (`src/input.h`) circular de 10 `InputFrame`s, alimentado a
  cada frame de render (`input_buffer.Push(...)` em `main.cpp`) e
  consultado de fato (`StepFighter` recebe `input_buffer.AtDelay(0)`).
- FSM completa (`FighterState`, 14 estados do requisito duro) em
  `fighter.h`; transição única e explícita em `ComputeNextState`
  (`fighter.cpp`). Estados sem gatilho ainda (`BlockStanding`,
  `BlockCrouching`, `Hitstun`, `Blockstun`, `Knockdown`, `Wakeup`, `Win`,
  `Lose`) existem no enum e no `switch`, mas são self-loop até F3/F4
  trazerem oponente/combate real.
- Ataque neutro com `AttackPhase` (Startup/Active/Recovery), não
  cancelável, indisponível no ar; cor do retângulo muda por fase
  (laranja/vermelho/violeta). Borda de subida do botão detectada via
  `Fighter::attack_button_held` (estado interno da sim, não do
  `InputFrame` — ver `docs/DECISOES.md`).
- `Fighter::facing_right` fixo em `true` por ora (sem P2 pra virar de
  frente); `WalkForward`/`WalkBackward` já usam essa semântica.
- Verificado com smoke test temporário (input+botão scriptados, log só em
  mudança de estado/fase, revertido antes do commit): parede, pulo
  (auto-hop com Up segurado, esperado), agachar, ataque com as 3 fases nos
  frames certos, e segurar o botão **não** gera ataques repetidos. Build
  limpo nos dois presets, ASan sem leak do nosso código.
- Ainda sem combate real (hitbox/hurtbox, dano, block de verdade) — isso é
  F3. Ainda um único lutador (sem P2, sem HUD) — isso é F4.

## Próxima fatia (F3 — Combate)

Extrair módulo `combat` (frame data table, hitbox/hurtbox por frame).
Tabela central de frame data para os golpes (por ora só o ataque neutro);
hurtboxes (corpo) e hitboxes (golpe) calculadas por frame a partir dessa
tabela; acerto gera dano, hitstun, pushback (ainda sem P2 real — pode
precisar de um segundo `Fighter` de teste/dummy só pra validar colisão de
caixas antes da F4 trazer o P2 jogável); defesa alta/baixa; blockstun;
chip damage. É aqui que os estados `BlockStanding`/`BlockCrouching`/
`Hitstun`/`Blockstun` da FSM ganham gatilho pela primeira vez.
