#include "combat.h"

namespace {

bool BoxesOverlap(Rectangle a, Rectangle b) {
  return a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height &&
         a.y + a.height > b.y;
}

bool CanBlock(const Fighter& fighter) {
  return fighter.is_grounded &&
         (fighter.state == FighterState::Idle || fighter.state == FighterState::WalkForward ||
          fighter.state == FighterState::WalkBackward || fighter.state == FighterState::Crouch);
}

}  // namespace

const MoveData& GetMoveData(MoveId move) {
  static constexpr MoveData kMoveTable[] = {
      // LightAttack: startup, active, recovery, dano, chip, hitstun,
      // blockstun, pushback (hit/block), hitbox (offset da posição do
      // atacante, largura, altura — assumindo facing_right).
      {6, 4, 10, 8, 1, 14, 8, 10.0f, 6.0f, Rectangle{kFighterHalfWidth, -90.0f, 50.0f, 30.0f}},
  };
  return kMoveTable[static_cast<int>(move)];
}

Rectangle FighterHurtbox(const Fighter& fighter) {
  const float height =
      fighter.state == FighterState::Crouch ? kFighterCrouchHeight : kFighterStandHeight;
  return Rectangle{fighter.position.x - kFighterHalfWidth, fighter.position.y - height,
                    kFighterHalfWidth * 2.0f, height};
}

Rectangle FighterHitbox(const Fighter& fighter, MoveId move) {
  const MoveData& data = GetMoveData(move);
  const float offset_x =
      fighter.facing_right ? data.hitbox.x : -(data.hitbox.x + data.hitbox.width);
  return Rectangle{fighter.position.x + offset_x, fighter.position.y + data.hitbox.y,
                    data.hitbox.width, data.hitbox.height};
}

void ResolveCombat(Fighter& attacker, Fighter& defender, const InputFrame& defender_input) {
  if (attacker.state != FighterState::Attack || attacker.attack_phase != AttackPhase::Active) {
    return;
  }
  if (attacker.current_attack_has_hit) return;

  constexpr MoveId kMove = MoveId::LightAttack;  // único golpe existente até a F5
  if (!BoxesOverlap(FighterHitbox(attacker, kMove), FighterHurtbox(defender))) return;

  attacker.current_attack_has_hit = true;
  const MoveData& data = GetMoveData(kMove);

  const bool attacker_is_to_my_right = attacker.position.x >= defender.position.x;
  const bool defender_holds_away = attacker_is_to_my_right
                                        ? DirectionHasLeft(defender_input.direction)
                                        : DirectionHasRight(defender_input.direction);
  const bool blocked = CanBlock(defender) && defender_holds_away;

  // Empurra o defensor pra longe do atacante; o atacante recua a metade
  // disso (pushback nos dois corpos, inclusive quando um está na parede —
  // o clamp de arena garante que ninguém atravessa).
  const float push_dir = attacker_is_to_my_right ? -1.0f : 1.0f;
  const float pushback = blocked ? data.pushback_block : data.pushback_hit;

  if (blocked) {
    defender.health -= data.chip_damage;
    ApplyBlockReaction(defender, data.blockstun_frames);
  } else {
    defender.health -= data.damage;
    ApplyHitReaction(defender, data.hitstun_frames);
  }
  defender.position.x += push_dir * pushback;
  attacker.position.x -= push_dir * pushback * 0.5f;

  ClampFighterToArena(defender);
  ClampFighterToArena(attacker);
}
