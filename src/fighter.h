#pragma once

#include "raylib.h"

#include <cstdint>

#include "input.h"
#include "stage.h"

// Dimensões do lutador (a arena em si vive em stage.h).
constexpr float kFighterHalfWidth = 30.0f;
constexpr float kFighterStandHeight = 120.0f;
constexpr float kFighterCrouchHeight = 80.0f;
constexpr float kFighterKnockdownHeight = 30.0f;

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

// Todo golpe existente vive na tabela central de frame data (combat.cpp)
// — nada de número mágico de dano/frame/hitbox espalhado pelo resto do
// código (requisito duro). Mora aqui (não em combat.h) porque `Fighter`
// precisa guardar qual golpe está em andamento. `Projectile` não usa
// hit/hurtbox do próprio lutador — vira uma entidade `Projectile`
// separada (projectile.h) no instante em que a fase Active começa.
enum class MoveId {
  LightStanding,
  MediumStanding,
  HeavyStanding,
  CrouchingLight,
  JumpingLight,
  Projectile,
};

// Só 2 personagens por ora (F5); diferença mecânica principal é o Gunner
// ter o projétil (agachado+pesado) — ver docs/DECISOES.md.
enum class CharacterId { Warrior, Gunner };

struct Fighter {
  Vector2 position{(kArenaLeft + kArenaRight) / 2.0f, kFloorY};
  Vector2 velocity{0.0f, 0.0f};
  bool is_grounded = true;
  bool facing_right = true;
  CharacterId character = CharacterId::Warrior;
  FighterState state = FighterState::Idle;
  AttackPhase attack_phase = AttackPhase::None;
  MoveId current_move = MoveId::LightStanding;  // golpe em andamento (só válido durante Attack)
  int state_timer = 0;               // frames decorridos no estado/fase atual
  int stun_target_frames = 0;        // duração alvo de Hitstun/Blockstun
  std::uint8_t buttons_held = 0;     // bitmask do frame anterior, p/ detectar borda de subida
  bool current_attack_has_hit = false;  // trava o golpe atual em 1 acerto só
  int health = 100;
  int combo_hits = 0;   // combo em andamento contra este lutador (reseta ao sair de Hitstun)
  int super_meter = 0;  // 0-100; enche ao dar/tomar dano (sem golpe pra gastar ainda — F5)
};

// Avança a simulação de um lutador em um passo fixo de 1/60s. Puro e
// determinístico: só lê o InputFrame recebido, nunca teclado/relógio/disco.
void StepFighter(Fighter& fighter, const InputFrame& input);

// Vira os dois lutadores um de frente pro outro, a partir da posição
// atual. Chamado antes de StepFighter/ResolveCombat: WalkForward/
// WalkBackward e o lado do hitbox dependem de `facing_right` estar
// sempre coerente com onde o oponente está.
void UpdateFacing(Fighter& a, Fighter& b);

// Mantém o lutador dentro dos limites da arena. Compartilhado entre a
// física (fighter.cpp) e o pushback de combate (combat.cpp).
void ClampFighterToArena(Fighter& fighter);

// Reações a um evento já confirmado por outro módulo (combate ou
// sistema de round). Continua sendo fighter.cpp quem escreve em
// `Fighter::state` — quem chama só informa o fato, nunca atribui o
// estado diretamente.
void ApplyHitReaction(Fighter& defender, int hitstun_frames);
void ApplyBlockReaction(Fighter& defender, int blockstun_frames);
void ApplyKnockdownReaction(Fighter& defender);
void SetRoundOutcome(Fighter& fighter, bool won);

// Soma (ou subtrai) do medidor de super, sempre travado em [0, 100].
// Chamado pelo módulo de combate quando um golpe acerta/é bloqueado — ver
// requisito duro "medidor ganha ao dar/tomar dano".
void AddSuperMeter(Fighter& fighter, int amount);

// Reseta um lutador pro início de um novo round (posição, vida, estado).
void ResetFighterForNewRound(Fighter& fighter, float start_x);
