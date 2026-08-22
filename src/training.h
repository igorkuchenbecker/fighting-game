#pragma once

// Modo treino (`--training`): P1 controla, P2 é um dummy infinito (volta
// pra vida cheia assim que zera, sem round/timer/tela de KO). Tecla F1
// alterna a exibição de hit/hurtbox; overlay de frame data do golpe
// atual do P1 fica sempre visível. Assume que a janela já foi criada
// (InitWindow) e roda até `WindowShouldClose()` — quem chama fecha a
// janela depois.
void RunTrainingMode();
