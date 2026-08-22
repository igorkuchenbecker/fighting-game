#pragma once

// Modo treino (`--training`): P1 controla, P2 é um dummy infinito (volta
// pra vida cheia assim que zera, sem round/timer/tela de KO). Tecla F1
// alterna a exibição de hit/hurtbox; overlay de frame data do golpe
// atual do P1 fica sempre visível. Assume que a janela já foi criada
// (InitWindow) e roda até `WindowShouldClose()` — quem chama fecha a
// janela depois.
//
// `use_ai_dummy` (default false, `--training` puro): dummy 100% parado,
// ideal pra estudar frame data/combos num alvo previsível. Com
// `--training-ai` (true): o dummy anda/bloqueia/ataca via IA determinística
// com seed (dummy_ai.h) — um modo separado, não substitui o padrão, pra
// não regredir o caso de uso original do treino (ver docs/DECISOES.md).
void RunTrainingMode(bool use_ai_dummy = false);
