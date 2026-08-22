# fighting-game

Fighting game 2D estilo SF/KOF escrito do zero em C++20 com raylib 5.5 — sem
engine, sem editor.

Dois lutadores com movesets distintos (leve/médio/pesado, antiaéreo, projétil e
super), partida best-of-3 com timer e HUD, modo treino com dummy infinito.

## Detalhes técnicos

- Timestep fixo 60Hz; simulação determinística — `--selftest` roda 3600 ticks
  duas vezes e compara o hash do estado final
- Input como dado puro (struct serializável) + buffer circular de 10 frames,
  pronto para rollback netcode
- FSM completa por lutador; startup/active/recovery de cada golpe definidos numa
  tabela central de frame data
- Hitbox/hurtbox por frame, pushback, blockstun/hitstun, knockdown/wakeup,
  medidor de super com invulnerabilidade no startup
- Debug build com ASan+UBSan; `-Wall -Wextra -Wpedantic -Werror` nos dois presets

## Build

```sh
cmake --preset release && cmake --build --preset release
./build/release/fighting_game            # partida
./build/release/fighting_game --training # treino (F1: hit/hurtbox + frame data)
./build/release/fighting_game --selftest # prova de determinismo (headless)
```

O primeiro preset compila o raylib via FetchContent (demora alguns minutos).

## Controles

| | P1 | P2 |
|---|---|---|
| Mover/pular/agachar | setas | WASD |
| Leve / médio / pesado | espaço / enter / shift direito | ctrl esq. / shift esq. / Q |

Gunner solta projétil com baixo+pesado; super = médio+pesado juntos com o
medidor cheio. Gamepads nos slots 0 e 1.

Estado detalhado do projeto: [AGENTS.md](AGENTS.md).
