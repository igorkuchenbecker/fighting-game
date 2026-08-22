#include "projectile.h"

#include "stage.h"

void SpawnProjectile(Projectile& projectile, Vector2 origin, bool facing_right) {
  projectile.active = true;
  projectile.position = origin;
  projectile.velocity_x = facing_right ? kProjectileSpeed : -kProjectileSpeed;
  projectile.lifetime_remaining = kProjectileLifetimeFrames;
}

void StepProjectile(Projectile& projectile) {
  if (!projectile.active) return;

  projectile.position.x += projectile.velocity_x;
  --projectile.lifetime_remaining;

  if (projectile.lifetime_remaining <= 0 || projectile.position.x < kArenaLeft ||
      projectile.position.x > kArenaRight) {
    projectile.active = false;
  }
}

Rectangle ProjectileHitbox(const Projectile& projectile) {
  return Rectangle{projectile.position.x - kProjectileHalfWidth,
                    projectile.position.y - kProjectileHalfHeight, kProjectileHalfWidth * 2.0f,
                    kProjectileHalfHeight * 2.0f};
}

void DrawProjectile(const Projectile& projectile) {
  if (!projectile.active) return;
  DrawRectangle(static_cast<int>(projectile.position.x - kProjectileHalfWidth),
                static_cast<int>(projectile.position.y - kProjectileHalfHeight),
                static_cast<int>(kProjectileHalfWidth * 2.0f),
                static_cast<int>(kProjectileHalfHeight * 2.0f), YELLOW);
}
