#pragma once

#include "raylib.h"

#include "input.h"

// Dimensões/limites da arena compartilhados entre a simulação (colisão) e
// o render (desenho do chão/paredes e da caixa do lutador).
constexpr float kArenaLeft = 100.0f;
constexpr float kArenaRight = 1180.0f;
constexpr float kFloorY = 600.0f;
constexpr float kFighterHalfWidth = 30.0f;

enum class FighterState {
  Idle,
  WalkForward,
  WalkBackward,
  Jump,
  Crouch,
  BlockStanding,
  BlockCrouching,
  Attack,
  Hitstun,
  Blockstun,
  Knockdown,
  Wakeup,
  Win,
  Lose,
};

enum class AttackPhase { None, Startup, Active, Recovery };

struct Fighter {
  Vector2 position{(kArenaLeft + kArenaRight) / 2.0f, kFloorY};
  Vector2 velocity{0.0f, 0.0f};
  bool is_grounded = true;
  bool facing_right = true;
  FighterState state = FighterState::Idle;
  AttackPhase attack_phase = AttackPhase::None;
  int state_timer = 0;         // frames decorridos no estado/fase atual
  bool attack_button_held = false;  // estado do frame anterior, p/ detectar borda de subida
};

// Avança a simulação de um lutador em um passo fixo de 1/60s. Puro e
// determinístico: só lê o InputFrame recebido, nunca teclado/relógio/disco.
void StepFighter(Fighter& fighter, const InputFrame& input);
