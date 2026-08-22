#pragma once

#include "raylib.h"

#include "fighter.h"
#include "input.h"
#include "projectile.h"

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

// Resultado de uma resolução de combate — permite que quem chamou (ex.:
// game.cpp) reaja a eventos (som de acerto/bloqueio) sem que a própria
// simulação precise saber de áudio/render. `None` = nada aconteceu
// (sem hitbox ativa, sem overlap, ou já tinha acertado).
enum class CombatOutcome { None, Hit, Blocked };

// Resolve combate de um par (atacante, defensor) em um step: se o
// atacante tem hitbox ativa (Attack em fase Active) sobrepondo a hurtbox
// do defensor e o golpe atual ainda não acertou, aplica dano/hitstun/
// pushback — ou, se o defensor estiver segurando "para trás" e puder
// bloquear, chip damage/blockstun/pushback reduzido no lugar. Não faz
// nada se o golpe atual for `MoveId::Projectile` (isso é
// ResolveProjectileHit, abaixo — o projétil vive fora do corpo do
// atacante).
CombatOutcome ResolveCombat(Fighter& attacker, Fighter& defender, const InputFrame& defender_input);

// Mesma lógica de dano/bloqueio/combo/medidor de ResolveCombat, mas pro
// hitbox de um projétil em voo contra o hurtbox do alvo. `owner` é quem
// lançou o projétil (ganha medidor de super no acerto); desativa o
// projétil ao conectar (só acerta 1 vez).
CombatOutcome ResolveProjectileHit(Projectile& projectile, Fighter& owner, Fighter& target,
                                    const InputFrame& target_input);
