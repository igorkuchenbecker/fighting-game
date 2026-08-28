# fighting-game

Fighting game 2D estilo Street Fighter/KOF escrito do zero em C++20 com raylib 5.5 — sem engine, sem editor.

## Finalidade

Um jogo de luta jogável e determinístico: dois lutadores com movesets distintos,
partida best-of-3 com timer e HUD, e um modo treino com dummy infinito. O foco é
uma base de código sólida para fazer um fighting game de verdade — simulação a
60Hz fixos, inputs tratados como dados, e netcode com rollback no futuro.

## Como funciona

- **Timestep fixo a 60Hz**: a simulação é determinística — `--selftest` roda 3600 ticks
  duas vezes e compara o hash do estado final
- **Input como dado puro**: struct serializável + buffer circular de 10 frames, pronto
  para rollback netcode
- **FSM completa por lutador**: startup/active/recovery de cada golpe definidos numa
  tabela central de frame data
- **Combate por frame**: hitbox/hurtbox, pushback, blockstun/hitstun, knockdown/wakeup
  e medidor de super com invulnerabilidade no startup
- **Debug confiável**: build debug com ASan+UBSan e `-Wall -Wextra -Wpedantic -Werror`
  nos dois presets (qualquer warning quebra a build)

## Como rodar

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

Gunner solta projétil com baixo+pesado; super = médio+pesado juntos com o medidor
cheio. Gamepads nos slots 0 e 1.