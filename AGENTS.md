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

**F0 — Fundação: CONCLUÍDA.**

- Projeto criado em `~/fighting-game/` (ver `docs/DECISOES.md` sobre o motivo).
- CMake + presets debug (ASan+UBSan)/release funcionando, raylib via
  FetchContent (tag `5.5`), zero warnings nos dois presets.
- `src/main.cpp`: janela 1280×720, loop de timestep fixo 60Hz com
  acumulador, imprime `tick` por passo de simulação, `BeginDrawing`/
  `EndDrawing` com fundo preto. Sem lógica de jogo ainda.
- Verificado manualmente: builda limpo (release e debug), roda a 60fps,
  `CloseWindow()` executa e ASan não acusa leak do nosso código (leaks do
  driver NVIDIA suprimidos — ver `docs/DECISOES.md`).
- Git inicializado, primeiro commit ainda por vir nesta mesma sessão.

## Próxima fatia (F1 — Lutador vivo)

Um retângulo controlável anda frente/trás, pula, agacha; arena com chão e
paredes; gravidade aplicada dentro do step de simulação (timestep fixo,
nunca por delta-time variável); câmera fixa. Ainda sem FSM explícita, sem
input buffer, sem combate — isso é F2/F3.
