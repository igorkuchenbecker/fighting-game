#pragma once

#include <cstdint>

#include "fighter.h"
#include "input.h"

// IA determinística e simples pro dummy do modo treino (--training-ai):
// anda em direção ou pra longe do jogador, bloqueia reativamente quando
// o jogador ataca por perto, e ataca de vez em quando. Toda decisão
// "aleatória" vem de um LCG com seed explícita — nunca rand()/time()
// (requisito duro).
struct DummyAi {
  std::uint32_t rng_state;
  int decision_timer = 0;
  Direction8 current_direction = Direction8::Neutral;
  bool pending_attack = false;
};

DummyAi MakeDummyAi(std::uint32_t seed);

// Decide o InputFrame do dummy pro tick atual. Puro/determinístico dado
// o estado de `ai` — não lê teclado/relógio, só o RNG interno da IA.
InputFrame ComputeDummyAiInput(DummyAi& ai, const Fighter& dummy, const Fighter& player);
