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
// `Fighter::state` fora daqui (exceto ApplyHitReaction/ApplyBlockReaction,
// que reagem a um evento de combate já confirmado, não a input).
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
      // Não cancelável no F2/F3: a saída é controlada pelo timer de fases
      // em AdvanceAttackPhase, não por input.
      return fighter.attack_phase == AttackPhase::None ? FighterState::Idle
                                                         : FighterState::Attack;

    case FighterState::Hitstun:
      return fighter.state_timer >= fighter.stun_target_frames ? FighterState::Idle
                                                                 : FighterState::Hitstun;

    case FighterState::Blockstun:
      return fighter.state_timer >= fighter.stun_target_frames ? FighterState::Idle
                                                                 : FighterState::Blockstun;

    // Sem gatilho ainda: BlockStanding/BlockCrouching viram guard-stance
    // visual de verdade só na F4 (quando há P2 segurando "pra trás" o
    // tempo todo, não só no instante do hit — ver docs/DECISOES.md).
    // Knockdown/Wakeup/Win/Lose chegam com o sistema de round da F4.
    case FighterState::BlockStanding:
    case FighterState::BlockCrouching:
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

// Estados em que o lutador ainda controla a própria velocidade horizontal
// pela direção segurada. Fora dessa lista (Crouch, Attack, Hitstun,
// Blockstun, e o que vier depois) o corpo fica travado horizontalmente,
// só sujeito a gravidade/pushback.
bool AllowsDirectionalMovement(FighterState state) {
  return state == FighterState::Idle || state == FighterState::WalkForward ||
         state == FighterState::WalkBackward || state == FighterState::Jump;
}

void ApplyPhysics(Fighter& fighter, bool wants_left, bool wants_right) {
  if (AllowsDirectionalMovement(fighter.state)) {
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

  ClampFighterToArena(fighter);
}

}  // namespace

void ClampFighterToArena(Fighter& fighter) {
  fighter.position.x = std::clamp(fighter.position.x, kArenaLeft + kFighterHalfWidth,
                                   kArenaRight - kFighterHalfWidth);
}

void ApplyHitReaction(Fighter& defender, int hitstun_frames) {
  defender.state = FighterState::Hitstun;
  defender.state_timer = 0;
  defender.stun_target_frames = hitstun_frames;
  defender.attack_phase = AttackPhase::None;
}

void ApplyBlockReaction(Fighter& defender, int blockstun_frames) {
  defender.state = FighterState::Blockstun;
  defender.state_timer = 0;
  defender.stun_target_frames = blockstun_frames;
  defender.attack_phase = AttackPhase::None;
}

void UpdateFacing(Fighter& a, Fighter& b) {
  a.facing_right = b.position.x >= a.position.x;
  b.facing_right = a.position.x >= b.position.x;
}

void ApplyKnockdownReaction(Fighter& defender) {
  defender.state = FighterState::Knockdown;
  defender.state_timer = 0;
  defender.velocity = Vector2{0.0f, 0.0f};
  defender.attack_phase = AttackPhase::None;
}

void SetRoundOutcome(Fighter& fighter, bool won) {
  fighter.state = won ? FighterState::Win : FighterState::Lose;
  fighter.state_timer = 0;
  fighter.velocity = Vector2{0.0f, 0.0f};
  fighter.attack_phase = AttackPhase::None;
}

void ResetFighterForNewRound(Fighter& fighter, float start_x) {
  fighter = Fighter{};
  fighter.position = Vector2{start_x, kFloorY};
}

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
    if (fighter.state == FighterState::Attack) {
      fighter.attack_phase = AttackPhase::Startup;
      fighter.current_attack_has_hit = false;
    } else {
      fighter.attack_phase = AttackPhase::None;
    }
  } else if (fighter.state == FighterState::Attack) {
    AdvanceAttackPhase(fighter);
  } else if (fighter.state == FighterState::Hitstun || fighter.state == FighterState::Blockstun) {
    ++fighter.state_timer;
  }

  ApplyPhysics(fighter, wants_left, wants_right);
}
