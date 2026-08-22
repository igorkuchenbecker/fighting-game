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
CMakeLists.txt       — alvo único fighting_game, FetchContent do raylib (tag 5.5)
CMakePresets.json     — presets debug/release (flags descritas acima)
src/main.cpp          — MONOLÍTICO por enquanto (regra: extrair módulos ao
                         passar de ~800 linhas OU na F2, nesta ordem:
                         input, fighter, combat, stage, game)
docs/DECISOES.md      — log de decisões (data | decisão | motivo | alternativas)
AGENTS.md / CLAUDE.md — este arquivo (symlink)
assets/               — ainda não existe; só entra na F5+ (spritesheets) e
                         F7 (áudio), sempre com fallback automático se ausente
```

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
**F1 — Lutador vivo: CONCLUÍDA.**

- `InputFrame { Direction8 direction; uint8_t buttons; }` já existe: a
  simulação (`StepFighter`) só recebe dado puro, nunca lê teclado — só
  `ReadDirection8()` toca a API do raylib, uma vez por iteração do loop
  externo (fora do `while` de steps fixos). Buffer circular de 10 frames
  ainda não existe (chega na F2).
- `Fighter { position, velocity, is_grounded, is_crouching }`: anda
  esquerda/direita (`kMoveSpeed`), pula (`kJumpVelocity`, só quando
  `is_grounded`), agacha (`is_grounded && wants_down`), gravidade
  (`kGravity`) aplicada todo step. Colisão com chão (`kFloorY`) e clamp
  nas paredes (`kArenaLeft`/`kArenaRight`) dentro de `StepFighter`.
- Câmera fixa implícita (mundo = tela, sem `Camera2D`); arena e retângulo
  do lutador desenhados em `DrawArena`/`DrawFighter`.
- Verificado com smoke test temporário (input scriptado + log a cada 50
  frames, revertido antes do commit): anda, bate e clampa na parede, pula
  e volta ao chão, agacha — depois voltou ao controle real por teclado
  (setas). Build limpo nos dois presets, ASan sem leak do nosso código.
- Ainda sem FSM explícita (transições são só `if`s dentro de
  `StepFighter`), sem input buffer, sem combate — isso é F2/F3.

## Próxima fatia (F2 — FSM + buffer)

FSM completa com transições explícitas (idle, walk_fwd, walk_back, jump,
crouch, block_standing, block_crouching, attack com startup/active/
recovery, hitstun, blockstun, knockdown, wakeup, win/lose — a maioria dos
estados só ganha sentido real a partir da F3, mas a FSM e suas transições
já devem existir); substituir a leitura direta de `Direction8` por um
buffer circular de 10 `InputFrame`s por jogador; ataque neutro com
startup/active/recovery visíveis (retângulo muda de cor nas fases). Nesta
fase (ou quando `main.cpp` passar de ~800 linhas) começa a extração de
módulos, nesta ordem: `input`, `fighter`, `combat`, `stage`, `game`.
