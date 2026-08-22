#include "selftest.h"

#include <cstdint>
#include <cstdio>

#include "game.h"

namespace {

constexpr int kSelfTestTicks = 3600;
constexpr std::uint32_t kP1Seed = 12345u;
constexpr std::uint32_t kP2Seed = 67890u;

// LCG simples (Numerical Recipes) — determinístico, não é
// rand()/relógio; existe só pra gerar variedade nos inputs do teste.
std::uint32_t NextLcg(std::uint32_t& state) {
  state = state * 1664525u + 1013904223u;
  return state;
}

InputFrame ScriptedInput(std::uint32_t& rng_state) {
  InputFrame frame;
  frame.direction = static_cast<Direction8>(NextLcg(rng_state) % 9);
  frame.buttons = static_cast<std::uint8_t>(NextLcg(rng_state) % 8);
  return frame;
}

std::uint64_t FnvHashBytes(std::uint64_t hash, const void* data, std::size_t size) {
  const auto* bytes = static_cast<const unsigned char*>(data);
  for (std::size_t i = 0; i < size; ++i) {
    hash ^= bytes[i];
    hash *= 1099511628211ULL;  // FNV-1a: prime
  }
  return hash;
}

// Hash campo a campo (não memcpy do struct inteiro) — bytes de padding
// não inicializados explicitamente poderiam variar entre execuções por
// lixo de pilha e gerar falso-positivo de "não determinístico".
template <typename T>
std::uint64_t HashField(std::uint64_t hash, const T& value) {
  return FnvHashBytes(hash, &value, sizeof(T));
}

std::uint64_t HashFighter(std::uint64_t hash, const Fighter& fighter) {
  hash = HashField(hash, fighter.position.x);
  hash = HashField(hash, fighter.position.y);
  hash = HashField(hash, fighter.velocity.x);
  hash = HashField(hash, fighter.velocity.y);
  hash = HashField(hash, fighter.is_grounded);
  hash = HashField(hash, fighter.facing_right);
  hash = HashField(hash, fighter.character);
  hash = HashField(hash, fighter.state);
  hash = HashField(hash, fighter.attack_phase);
  hash = HashField(hash, fighter.current_move);
  hash = HashField(hash, fighter.state_timer);
  hash = HashField(hash, fighter.stun_target_frames);
  hash = HashField(hash, fighter.buttons_held);
  hash = HashField(hash, fighter.current_attack_has_hit);
  hash = HashField(hash, fighter.health);
  hash = HashField(hash, fighter.combo_hits);
  hash = HashField(hash, fighter.super_meter);
  return hash;
}

std::uint64_t HashProjectile(std::uint64_t hash, const Projectile& projectile) {
  hash = HashField(hash, projectile.active);
  hash = HashField(hash, projectile.position.x);
  hash = HashField(hash, projectile.position.y);
  hash = HashField(hash, projectile.velocity_x);
  hash = HashField(hash, projectile.lifetime_remaining);
  return hash;
}

std::uint64_t HashMatch(std::uint64_t hash, const Match& match) {
  hash = HashField(hash, match.round_number);
  hash = HashField(hash, match.wins_p1);
  hash = HashField(hash, match.wins_p2);
  hash = HashField(hash, match.timer_seconds);
  hash = HashField(hash, match.phase);
  hash = HashField(hash, match.end_reason);
  hash = HashField(hash, match.phase_timer);
  hash = HashProjectile(hash, match.p1_projectile);
  hash = HashProjectile(hash, match.p2_projectile);
  return hash;
}

std::uint64_t RunDeterministicSimulation() {
  Fighter p1;
  Fighter p2;
  p2.character = CharacterId::Gunner;
  ResetFighterForNewRound(p1, kArenaLeft + 250.0f);
  ResetFighterForNewRound(p2, kArenaRight - 250.0f);
  Match match;

  std::uint32_t p1_rng = kP1Seed;
  std::uint32_t p2_rng = kP2Seed;

  for (int tick = 0; tick < kSelfTestTicks; ++tick) {
    UpdateMatch(match, p1, p2, ScriptedInput(p1_rng), ScriptedInput(p2_rng));
  }

  constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
  std::uint64_t hash = kFnvOffsetBasis;
  hash = HashFighter(hash, p1);
  hash = HashFighter(hash, p2);
  hash = HashMatch(hash, match);
  return hash;
}

}  // namespace

int RunSelfTest() {
  const std::uint64_t hash1 = RunDeterministicSimulation();
  const std::uint64_t hash2 = RunDeterministicSimulation();

  if (hash1 == hash2) {
    std::printf("--selftest: DETERMINISTICO OK (%d ticks x2, hash=%016llx)\n", kSelfTestTicks,
                static_cast<unsigned long long>(hash1));
    return 0;
  }

  std::fprintf(stderr, "--selftest: FALHA DE DETERMINISMO (hash1=%016llx hash2=%016llx)\n",
               static_cast<unsigned long long>(hash1), static_cast<unsigned long long>(hash2));
  return 1;
}
