#include "fighter.h"

#include <algorithm>

#include "combat.h"  // GetMoveData: fighter.cpp só LÊ a tabela central de frame data

namespace {

constexpr float kMoveSpeed = 6.0f;
constexpr float kJumpVelocity = -16.0f;
constexpr float kGravity = 0.8f;

// Decide qual golpe entra quando o ataque é acionado: agachado sempre usa
// o golpe baixo (1 golpe agachado só, não uma matriz leve/médio/pesado
// agachados); em pé, respeita a força do botão pressionado. Pesado dobra
// de antiaéreo (hitbox alto, ver combat.cpp) em vez de ganhar um golpe
// dedicado — ver docs/DECISOES.md.
MoveId DetermineMove(CharacterId character, FighterState state_before_attack, bool heavy_pressed,
                     bool medium_pressed, bool wants_super, int super_meter) {
  // Super tem prioridade sobre qualquer outro golpe: segurar médio+pesado
  // com o medidor cheio sempre vira Super, não importa o estado/força.
  if (wants_super && super_meter >= kSuperMeterMax) return MoveId::Super;
  if (state_before_attack == FighterState::Jump) return MoveId::JumpingLight;
  if (state_before_attack == FighterState::Crouch) {
    // Gunner troca o agachado padrão pelo projétil quando segura pesado;
    // Warrior não tem projétil, sempre agachado normal.
    if (character == CharacterId::Gunner && heavy_pressed) return MoveId::Projectile;
    return MoveId::CrouchingLight;
  }
  if (heavy_pressed) return MoveId::HeavyStanding;
  if (medium_pressed) return MoveId::MediumStanding;
  return MoveId::LightStanding;
}

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
      // `is_grounded` só vira true de novo depois de ApplyPhysics pousar o
      // lutador (tick seguinte); enquanto ainda no ar, dá pra atacar.
      if (attack_just_pressed) return FighterState::Attack;
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
  const MoveData& data = GetMoveData(fighter.current_move);
  ++fighter.state_timer;
  switch (fighter.attack_phase) {
    case AttackPhase::Startup:
      if (fighter.state_timer >= data.startup_frames) {
        fighter.attack_phase = AttackPhase::Active;
        fighter.state_timer = 0;
      }
      break;
    case AttackPhase::Active:
      if (fighter.state_timer >= data.active_frames) {
        fighter.attack_phase = AttackPhase::Recovery;
        fighter.state_timer = 0;
      }
      break;
    case AttackPhase::Recovery:
      if (fighter.state_timer >= data.recovery_frames) {
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
  const CharacterId character = fighter.character;  // sobrevive ao reset, não é estado de round
  fighter = Fighter{};
  fighter.character = character;
  fighter.position = Vector2{start_x, kFloorY};
}

void AddSuperMeter(Fighter& fighter, int amount) {
  fighter.super_meter = std::clamp(fighter.super_meter + amount, 0, kSuperMeterMax);
}

bool IsInvulnerable(const Fighter& fighter) {
  return fighter.state == FighterState::Attack && fighter.current_move == MoveId::Super &&
         fighter.attack_phase == AttackPhase::Startup;
}

void StepFighter(Fighter& fighter, const InputFrame& input) {
  const bool wants_left = DirectionHasLeft(input.direction);
  const bool wants_right = DirectionHasRight(input.direction);
  const bool wants_up = DirectionHasUp(input.direction);
  const bool wants_down = DirectionHasDown(input.direction);

  const auto just_pressed = static_cast<std::uint8_t>(input.buttons & ~fighter.buttons_held);
  fighter.buttons_held = input.buttons;
  const bool light_just_pressed = (just_pressed & kButtonLight) != 0;
  const bool medium_just_pressed = (just_pressed & kButtonMedium) != 0;
  const bool heavy_just_pressed = (just_pressed & kButtonHeavy) != 0;
  const bool any_attack_just_pressed = light_just_pressed || medium_just_pressed || heavy_just_pressed;
  // Segurar médio+pesado juntos (não precisa dos dois no mesmo tick, só
  // os dois DOWN agora) é o gatilho do super — sem motion input tipo
  // quarter-circle implementado ainda, ver docs/DECISOES.md.
  const bool wants_super = (input.buttons & (kButtonMedium | kButtonHeavy)) ==
                            (kButtonMedium | kButtonHeavy);

  const FighterState previous_state = fighter.state;
  fighter.state = ComputeNextState(fighter, wants_left, wants_right, wants_up, wants_down,
                                    any_attack_just_pressed);

  if (fighter.state != previous_state) {
    fighter.state_timer = 0;
    if (fighter.state == FighterState::Attack) {
      fighter.attack_phase = AttackPhase::Startup;
      fighter.current_attack_has_hit = false;
      fighter.current_move = DetermineMove(fighter.character, previous_state, heavy_just_pressed,
                                            medium_just_pressed, wants_super, fighter.super_meter);
      if (fighter.current_move == MoveId::Super) {
        fighter.super_meter = 0;  // consome o medidor inteiro ao ativar
      }
    } else {
      fighter.attack_phase = AttackPhase::None;
    }
    if (previous_state == FighterState::Hitstun && fighter.state != FighterState::Hitstun) {
      fighter.combo_hits = 0;  // combo termina quando o defensor volta a agir
    }
  } else if (fighter.state == FighterState::Attack) {
    AdvanceAttackPhase(fighter);
  } else if (fighter.state == FighterState::Hitstun || fighter.state == FighterState::Blockstun) {
    ++fighter.state_timer;
  }

  ApplyPhysics(fighter, wants_left, wants_right);
}
