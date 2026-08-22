#pragma once

#include "raylib.h"

#include "input.h"

// Dimensões/limites da arena compartilhados entre a simulação (colisão,
// caixas de combate) e o render (desenho do chão/paredes/lutador).
constexpr float kArenaLeft = 100.0f;
constexpr float kArenaRight = 1180.0f;
constexpr float kFloorY = 600.0f;
constexpr float kFighterHalfWidth = 30.0f;
constexpr float kFighterStandHeight = 120.0f;
constexpr float kFighterCrouchHeight = 80.0f;

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
  int state_timer = 0;               // frames decorridos no estado/fase atual
  int stun_target_frames = 0;        // duração alvo de Hitstun/Blockstun (F3)
  bool attack_button_held = false;   // estado do frame anterior, p/ detectar borda de subida
  bool current_attack_has_hit = false;  // trava o golpe atual em 1 acerto só
  int health = 100;
};

// Avança a simulação de um lutador em um passo fixo de 1/60s. Puro e
// determinístico: só lê o InputFrame recebido, nunca teclado/relógio/disco.
void StepFighter(Fighter& fighter, const InputFrame& input);

// Mantém o lutador dentro dos limites da arena. Compartilhado entre a
// física (fighter.cpp) e o pushback de combate (combat.cpp).
void ClampFighterToArena(Fighter& fighter);

// Reações a um golpe confirmado pelo módulo de combate. Continuam sendo
// fighter.cpp quem escreve em `Fighter::state` — combat.cpp só informa o
// fato ("fui atingido"/"bloqueei"), nunca atribui o estado diretamente.
void ApplyHitReaction(Fighter& defender, int hitstun_frames);
void ApplyBlockReaction(Fighter& defender, int blockstun_frames);
