#include "dummy_ai.h"

namespace {

constexpr float kBlockReactionRange = 150.0f;  // distância pra reagir bloqueando
constexpr float kAttackRange = 120.0f;         // distância pra decidir atacar
constexpr int kMinDecisionFrames = 30;
constexpr int kDecisionFramesSpread = 30;  // duração de cada decisão: [30, 59] frames

// LCG simples (mesma família do selftest.cpp) — determinístico, nunca
// rand()/relógio.
std::uint32_t NextLcg(std::uint32_t& state) {
  state = state * 1664525u + 1013904223u;
  return state;
}

float AbsDistance(float a, float b) {
  const float d = a - b;
  return d < 0.0f ? -d : d;
}

}  // namespace

DummyAi MakeDummyAi(std::uint32_t seed) {
  DummyAi ai;
  ai.rng_state = seed;
  return ai;
}

InputFrame ComputeDummyAiInput(DummyAi& ai, const Fighter& dummy, const Fighter& player) {
  // Reage bloqueando se o jogador está atacando por perto — isso tem
  // prioridade sobre qualquer decisão "aleatória" em andamento.
  const bool player_attacking_close = player.state == FighterState::Attack &&
      player.attack_phase == AttackPhase::Active &&
      AbsDistance(player.position.x, dummy.position.x) < kBlockReactionRange;
  if (player_attacking_close) {
    InputFrame frame;
    const bool player_is_to_my_right = player.position.x >= dummy.position.x;
    frame.direction = player_is_to_my_right ? Direction8::Left : Direction8::Right;
    return frame;
  }

  --ai.decision_timer;
  if (ai.decision_timer <= 0) {
    const std::uint32_t roll = NextLcg(ai.rng_state) % 100;
    ai.decision_timer =
        kMinDecisionFrames + static_cast<int>(NextLcg(ai.rng_state) % kDecisionFramesSpread);
    ai.pending_attack = false;

    const bool player_is_to_my_right = player.position.x >= dummy.position.x;
    if (roll < 35) {
      ai.current_direction = player_is_to_my_right ? Direction8::Right : Direction8::Left;  // aproxima
    } else if (roll < 55) {
      ai.current_direction = player_is_to_my_right ? Direction8::Left : Direction8::Right;  // afasta
    } else if (roll < 80) {
      ai.current_direction = Direction8::Neutral;
    } else {
      ai.current_direction = Direction8::Neutral;
      ai.pending_attack = true;
    }
  }

  InputFrame frame;
  frame.direction = ai.current_direction;
  if (ai.pending_attack && AbsDistance(player.position.x, dummy.position.x) < kAttackRange) {
    frame.buttons = kButtonLight;
    ai.pending_attack = false;  // só um pulso de borda de subida, não segura o botão
  }
  return frame;
}
