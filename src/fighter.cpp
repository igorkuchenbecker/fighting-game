#include "fighter.h"

#include <algorithm>

namespace {

constexpr float kMoveSpeed = 6.0f;
constexpr float kJumpVelocity = -16.0f;
constexpr float kGravity = 0.8f;

// Frame data do ataque neutro — única normal existente até a F3, quando
// vira uma tabela central de golpes (startup/active/recovery/dano/etc.).
constexpr int kAttackStartupFrames = 6;
constexpr int kAttackActiveFrames = 4;
constexpr int kAttackRecoveryFrames = 10;

// Decide o próximo estado da FSM a partir do estado atual + fatos do
// frame (direção pedida, borda de subida do botão de ataque, se está no
// chão). Único ponto de transição — nenhum outro lugar do código muda
// `Fighter::state` fora daqui.
FighterState ComputeNextState(const Fighter& fighter, bool wants_left, bool wants_right,
                               bool wants_up, bool wants_down, bool attack_just_pressed) {
  switch (fighter.state) {
    case FighterState::Idle:
    case FighterState::WalkForward:
    case FighterState::WalkBackward:
    case FighterState::Crouch:
      if (attack_just_pressed) return FighterState::Attack;
      if (wants_up) return FighterState::Jump;
      if (wants_down) return FighterState::Crouch;
      if (wants_left && !wants_right) {
        return fighter.facing_right ? FighterState::WalkBackward : FighterState::WalkForward;
      }
      if (wants_right && !wants_left) {
        return fighter.facing_right ? FighterState::WalkForward : FighterState::WalkBackward;
      }
      return FighterState::Idle;

    case FighterState::Jump:
      return fighter.is_grounded ? FighterState::Idle : FighterState::Jump;

    case FighterState::Attack:
      // Não cancelável no F2: a saída é controlada pelo timer de fases em
      // AdvanceAttackPhase, não por input.
      return fighter.attack_phase == AttackPhase::None ? FighterState::Idle
                                                         : FighterState::Attack;

    // Sem gatilho ainda: chegam com o combate/oponente real (F3/F4).
    case FighterState::BlockStanding:
    case FighterState::BlockCrouching:
    case FighterState::Hitstun:
    case FighterState::Blockstun:
    case FighterState::Knockdown:
    case FighterState::Wakeup:
    case FighterState::Win:
    case FighterState::Lose:
      return fighter.state;
  }
  return fighter.state;  // inalcançável; silencia -Wreturn-type
}

void AdvanceAttackPhase(Fighter& fighter) {
  ++fighter.state_timer;
  switch (fighter.attack_phase) {
    case AttackPhase::Startup:
      if (fighter.state_timer >= kAttackStartupFrames) {
        fighter.attack_phase = AttackPhase::Active;
        fighter.state_timer = 0;
      }
      break;
    case AttackPhase::Active:
      if (fighter.state_timer >= kAttackActiveFrames) {
        fighter.attack_phase = AttackPhase::Recovery;
        fighter.state_timer = 0;
      }
      break;
    case AttackPhase::Recovery:
      if (fighter.state_timer >= kAttackRecoveryFrames) {
        fighter.attack_phase = AttackPhase::None;
        fighter.state_timer = 0;
      }
      break;
    case AttackPhase::None:
      break;
  }
}

void ApplyPhysics(Fighter& fighter, bool wants_left, bool wants_right) {
  const bool can_move_horizontally =
      fighter.state != FighterState::Crouch && fighter.state != FighterState::Attack;

  if (can_move_horizontally) {
    if (wants_left && !wants_right) {
      fighter.velocity.x = -kMoveSpeed;
    } else if (wants_right && !wants_left) {
      fighter.velocity.x = kMoveSpeed;
    } else {
      fighter.velocity.x = 0.0f;
    }
  } else {
    fighter.velocity.x = 0.0f;
  }

  if (fighter.state == FighterState::Jump && fighter.is_grounded) {
    fighter.velocity.y = kJumpVelocity;
    fighter.is_grounded = false;
  }

  fighter.velocity.y += kGravity;

  fighter.position.x += fighter.velocity.x;
  fighter.position.y += fighter.velocity.y;

  if (fighter.position.y >= kFloorY) {
    fighter.position.y = kFloorY;
    fighter.velocity.y = 0.0f;
    fighter.is_grounded = true;
  }

  fighter.position.x = std::clamp(fighter.position.x, kArenaLeft + kFighterHalfWidth,
                                   kArenaRight - kFighterHalfWidth);
}

}  // namespace

void StepFighter(Fighter& fighter, const InputFrame& input) {
  const bool wants_left = DirectionHasLeft(input.direction);
  const bool wants_right = DirectionHasRight(input.direction);
  const bool wants_up = DirectionHasUp(input.direction);
  const bool wants_down = DirectionHasDown(input.direction);

  const bool attack_down = (input.buttons & kButtonLight) != 0;
  const bool attack_just_pressed = attack_down && !fighter.attack_button_held;
  fighter.attack_button_held = attack_down;

  const FighterState previous_state = fighter.state;
  fighter.state =
      ComputeNextState(fighter, wants_left, wants_right, wants_up, wants_down, attack_just_pressed);

  if (fighter.state != previous_state) {
    fighter.state_timer = 0;
    fighter.attack_phase =
        fighter.state == FighterState::Attack ? AttackPhase::Startup : AttackPhase::None;
  }

  if (fighter.state == FighterState::Attack) {
    AdvanceAttackPhase(fighter);
  }

  ApplyPhysics(fighter, wants_left, wants_right);
}
