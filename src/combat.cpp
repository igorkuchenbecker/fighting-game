#include "combat.h"

#include <algorithm>

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

// Ganho de medidor de super por hit/bloqueio — genérico por ora (só existe
// 1 golpe); quando a F5 trouxer um moveset de verdade, cada golpe pode
// ganhar seu próprio valor dentro de MoveData.
constexpr int kSuperGainOnHitAttacker = 10;
constexpr int kSuperGainOnHitDefender = 5;
constexpr int kSuperGainOnBlockAttacker = 5;
constexpr int kSuperGainOnBlockDefender = 2;

// Escala de dano por golpe consecutivo dentro do mesmo combo (requisito
// duro "combo counter com escala de dano"): 100% no 1º hit, -10% por hit
// adicional, com piso de 50%.
float ComboDamageScale(int combo_hits) {
  return std::max(0.5f, 1.0f - 0.1f * static_cast<float>(combo_hits - 1));
}

}  // namespace

const MoveData& GetMoveData(MoveId move) {
  // Campos: startup, active, recovery, dano, chip, hitstun, blockstun,
  // pushback (hit/block), hitbox (offset x/y da posição do atacante,
  // largura, altura — assumindo facing_right). Ordem tem que bater com
  // MoveId (fighter.h). Leve rápido/fraco, pesado lento/forte (ainda
  // placeholder — balanceamento de verdade é F5 mais adiante).
  static constexpr MoveData kMoveTable[] = {
      // LightStanding
      {6, 4, 10, 8, 1, 14, 8, 10.0f, 6.0f, Rectangle{kFighterHalfWidth, -90.0f, 50.0f, 30.0f}},
      // MediumStanding
      {9, 5, 14, 13, 2, 18, 10, 12.0f, 7.0f, Rectangle{kFighterHalfWidth, -95.0f, 60.0f, 35.0f}},
      // HeavyStanding — dobra de antiaéreo: hitbox alto/alongado verticalmente
      {13, 6, 20, 18, 3, 24, 14, 16.0f, 9.0f, Rectangle{kFighterHalfWidth, -170.0f, 55.0f, 110.0f}},
      // CrouchingLight — golpe agachado, hitbox baixo
      {5, 4, 9, 6, 1, 12, 7, 8.0f, 5.0f, Rectangle{kFighterHalfWidth, -40.0f, 45.0f, 30.0f}},
      // JumpingLight — único golpe aéreo por ora, hitbox na altura do peito
      {5, 6, 8, 10, 2, 16, 9, 9.0f, 5.0f, Rectangle{kFighterHalfWidth, -70.0f, 55.0f, 50.0f}},
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

  const MoveId move = attacker.current_move;
  if (!BoxesOverlap(FighterHitbox(attacker, move), FighterHurtbox(defender))) return;

  attacker.current_attack_has_hit = true;
  const MoveData& data = GetMoveData(move);

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
    AddSuperMeter(attacker, kSuperGainOnBlockAttacker);
    AddSuperMeter(defender, kSuperGainOnBlockDefender);
  } else {
    const bool is_combo_continuation = defender.state == FighterState::Hitstun;
    defender.combo_hits = is_combo_continuation ? defender.combo_hits + 1 : 1;
    const int scaled_damage =
        static_cast<int>(static_cast<float>(data.damage) * ComboDamageScale(defender.combo_hits));
    defender.health -= scaled_damage;
    ApplyHitReaction(defender, data.hitstun_frames);
    AddSuperMeter(attacker, kSuperGainOnHitAttacker);
    AddSuperMeter(defender, kSuperGainOnHitDefender);
  }
  defender.position.x += push_dir * pushback;
  attacker.position.x -= push_dir * pushback * 0.5f;

  ClampFighterToArena(defender);
  ClampFighterToArena(attacker);
}
