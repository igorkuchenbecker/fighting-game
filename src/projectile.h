#pragma once

#include "raylib.h"

// Entidade independente do Fighter que a lançou: viaja em linha reta,
// expira por tempo de vida ou ao sair da arena. Cada jogador tem no
// máximo 1 projétil ativo por vez (ver Match em game.h).
struct Projectile {
  bool active = false;
  Vector2 position{0.0f, 0.0f};
  float velocity_x = 0.0f;
  int lifetime_remaining = 0;
};

constexpr float kProjectileSpeed = 10.0f;
constexpr int kProjectileLifetimeFrames = 90;  // ~1.5s a 60Hz
constexpr float kProjectileHalfWidth = 12.0f;
constexpr float kProjectileHalfHeight = 12.0f;

void SpawnProjectile(Projectile& projectile, Vector2 origin, bool facing_right);

// Avança um passo fixo: posição, tempo de vida, desativa ao expirar ou
// sair da arena. Puro/determinístico como StepFighter.
void StepProjectile(Projectile& projectile);

Rectangle ProjectileHitbox(const Projectile& projectile);

void DrawProjectile(const Projectile& projectile);
