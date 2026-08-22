#pragma once

#include "raylib.h"

#include "fighter.h"
#include "input.h"

// `MoveId` mora em fighter.h (Fighter::current_move precisa dele). Todo
// golpe existente vive na tabela abaixo — nada de número mágico de
// dano/frame/hitbox espalhado pelo resto do código (requisito duro).
struct MoveData {
  int startup_frames;
  int active_frames;
  int recovery_frames;
  int damage;
  int chip_damage;
  int hitstun_frames;
  int blockstun_frames;
  float pushback_hit;
  float pushback_block;
  Rectangle hitbox;  // offset relativo à posição do atacante, assumindo facing_right
};

const MoveData& GetMoveData(MoveId move);

Rectangle FighterHurtbox(const Fighter& fighter);
Rectangle FighterHitbox(const Fighter& fighter, MoveId move);

// Resolve combate de um par (atacante, defensor) em um step: se o
// atacante tem hitbox ativa (Attack em fase Active) sobrepondo a hurtbox
// do defensor e o golpe atual ainda não acertou, aplica dano/hitstun/
// pushback — ou, se o defensor estiver segurando "para trás" e puder
// bloquear, chip damage/blockstun/pushback reduzido no lugar.
void ResolveCombat(Fighter& attacker, Fighter& defender, const InputFrame& defender_input);
