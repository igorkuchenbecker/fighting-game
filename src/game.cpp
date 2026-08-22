#include "game.h"

#include <algorithm>

#include "raylib.h"

namespace {

constexpr int kHudBarWidth = 380;
constexpr int kHudBarHeight = 24;
constexpr int kHudMeterHeight = 10;
constexpr int kHudMeterGap = 4;
constexpr int kHudPortraitSize = 40;
constexpr int kHudMargin = 30;
constexpr int kHudBarY = 70;

Color FighterColor(const Fighter& fighter, Color base_color) {
  if (IsInvulnerable(fighter)) return WHITE;  // janela de invulnerabilidade do super
  if (fighter.state == FighterState::Attack) {
    switch (fighter.attack_phase) {
      case AttackPhase::Startup:
        return ORANGE;
      case AttackPhase::Active:
        return RED;
      case AttackPhase::Recovery:
        return VIOLET;
      case AttackPhase::None:
        break;
    }
  }
  if (fighter.state == FighterState::Hitstun) return YELLOW;
  if (fighter.state == FighterState::Blockstun) return SKYBLUE;
  if (fighter.state == FighterState::Knockdown) return DARKGRAY;
  if (fighter.state == FighterState::Win) return GOLD;
  if (fighter.state == FighterState::Lose) return DARKGRAY;
  return base_color;
}

float FighterDrawHeight(const Fighter& fighter) {
  if (fighter.state == FighterState::Knockdown) return kFighterKnockdownHeight;
  if (fighter.state == FighterState::Crouch) return kFighterCrouchHeight;
  return kFighterStandHeight;
}

void EndRound(Match& match, Fighter& p1, Fighter& p2) {
  const bool p1_won = p2.health <= p1.health;
  Fighter& winner = p1_won ? p1 : p2;
  Fighter& loser = p1_won ? p2 : p1;

  SetRoundOutcome(winner, /*won=*/true);
  if (match.end_reason == RoundEndReason::Knockout) {
    ApplyKnockdownReaction(loser);  // fica caído durante a tela de KO
  } else {
    SetRoundOutcome(loser, /*won=*/false);
  }

  if (p1_won) {
    ++match.wins_p1;
  } else {
    ++match.wins_p2;
  }

  match.phase =
      (match.wins_p1 >= kWinsNeeded || match.wins_p2 >= kWinsNeeded) ? RoundPhase::MatchOver
                                                                      : RoundPhase::Ko;
  match.phase_timer = 0;
}

void StartNextRound(Match& match, Fighter& p1, Fighter& p2) {
  ++match.round_number;
  ResetFighterForNewRound(p1, kArenaLeft + 250.0f);
  ResetFighterForNewRound(p2, kArenaRight - 250.0f);
  match.timer_seconds = kRoundTimeSeconds;
  match.end_reason = RoundEndReason::None;
  match.phase = RoundPhase::Intro;
  match.phase_timer = 0;
  match.p1_projectile = Projectile{};
  match.p2_projectile = Projectile{};
}

// Spawna o projétil no exato tick em que a fase Active começa (borda de
// subida — `phase_before` é o attack_phase ANTES do StepFighter deste
// tick). Só dispara se o golpe em andamento for MoveId::Projectile.
void MaybeSpawnProjectile(const Fighter& fighter, Projectile& projectile, AttackPhase phase_before) {
  if (fighter.current_move != MoveId::Projectile) return;
  if (fighter.attack_phase != AttackPhase::Active || phase_before == AttackPhase::Active) return;

  const float offset_x = fighter.facing_right ? kFighterHalfWidth : -kFighterHalfWidth;
  const Vector2 origin{fighter.position.x + offset_x, fighter.position.y - 60.0f};
  SpawnProjectile(projectile, origin, fighter.facing_right);
}

void DrawCenteredText(const char* text, int y, int font_size, Color color) {
  const int width = MeasureText(text, font_size);
  DrawText(text, (kScreenWidth - width) / 2, y, font_size, color);
}

// `anchor_right`: a barra de P2 fica presa na borda direita e encolhe
// pra dentro (drena da esquerda pra direita), espelhando a de P1.
void DrawStatBar(int bar_x, int y, int height, float fraction, Color empty_color,
                  Color fill_color, bool anchor_right) {
  fraction = std::clamp(fraction, 0.0f, 1.0f);
  const int filled_width = static_cast<int>(static_cast<float>(kHudBarWidth) * fraction);
  const int filled_x = anchor_right ? bar_x + (kHudBarWidth - filled_width) : bar_x;
  DrawRectangle(bar_x, y, kHudBarWidth, height, empty_color);
  DrawRectangle(filled_x, y, filled_width, height, fill_color);
  DrawRectangleLines(bar_x, y, kHudBarWidth, height, RAYWHITE);
}

void DrawComboText(int bar_x, int y, int combo_hits, bool anchor_right) {
  if (combo_hits < 2) return;
  const char* text = TextFormat("%d HITS", combo_hits);
  const int width = MeasureText(text, 20);
  DrawText(text, anchor_right ? bar_x + kHudBarWidth - width : bar_x, y, 20, YELLOW);
}

}  // namespace

MatchEvents UpdateMatch(Match& match, Fighter& p1, Fighter& p2, const InputFrame& p1_input,
                         const InputFrame& p2_input) {
  MatchEvents events;
  ++match.phase_timer;

  switch (match.phase) {
    case RoundPhase::Intro:
      if (match.phase_timer >= kIntroFrames) {
        match.phase = RoundPhase::Fighting;
        match.phase_timer = 0;
      }
      break;

    case RoundPhase::Fighting: {
      UpdateFacing(p1, p2);
      const FighterState p1_state_before = p1.state;
      const FighterState p2_state_before = p2.state;
      const AttackPhase p1_phase_before = p1.attack_phase;
      const AttackPhase p2_phase_before = p2.attack_phase;
      StepFighter(p1, p1_input);
      StepFighter(p2, p2_input);
      events.p1_jumped = p1_state_before != FighterState::Jump && p1.state == FighterState::Jump;
      events.p2_jumped = p2_state_before != FighterState::Jump && p2.state == FighterState::Jump;

      MaybeSpawnProjectile(p1, match.p1_projectile, p1_phase_before);
      MaybeSpawnProjectile(p2, match.p2_projectile, p2_phase_before);

      StepProjectile(match.p1_projectile);
      StepProjectile(match.p2_projectile);

      // Resolvido nos dois sentidos: qualquer jogador pode ser o atacante.
      const CombatOutcome p1_attacks_p2 = ResolveCombat(p1, p2, p2_input);
      const CombatOutcome p2_attacks_p1 = ResolveCombat(p2, p1, p1_input);
      const CombatOutcome proj1_hits_p2 = ResolveProjectileHit(match.p1_projectile, p1, p2, p2_input);
      const CombatOutcome proj2_hits_p1 = ResolveProjectileHit(match.p2_projectile, p2, p1, p1_input);

      events.p2_hit_landed = p1_attacks_p2 == CombatOutcome::Hit || proj1_hits_p2 == CombatOutcome::Hit;
      events.p2_hit_blocked =
          p1_attacks_p2 == CombatOutcome::Blocked || proj1_hits_p2 == CombatOutcome::Blocked;
      events.p1_hit_landed = p2_attacks_p1 == CombatOutcome::Hit || proj2_hits_p1 == CombatOutcome::Hit;
      events.p1_hit_blocked =
          p2_attacks_p1 == CombatOutcome::Blocked || proj2_hits_p1 == CombatOutcome::Blocked;

      match.timer_seconds -= static_cast<float>(kFixedDt);

      if (p1.health <= 0 || p2.health <= 0) {
        match.end_reason = RoundEndReason::Knockout;
        events.knockout_happened = true;
        EndRound(match, p1, p2);
      } else if (match.timer_seconds <= 0.0f) {
        match.timer_seconds = 0.0f;
        match.end_reason = RoundEndReason::Timeout;
        EndRound(match, p1, p2);
      }
      break;
    }

    case RoundPhase::Ko:
      if (match.phase_timer >= kKoFrames) {
        StartNextRound(match, p1, p2);
      }
      break;

    case RoundPhase::MatchOver:
      break;  // fica parado; sem menu de "jogar de novo" ainda (fora do escopo da F4)
  }

  return events;
}

void DrawFighter(const Fighter& fighter, Color base_color, const CharacterSprite* sprite) {
  const float height = FighterDrawHeight(fighter);
  const Color color = FighterColor(fighter, base_color);

  if (sprite != nullptr && sprite->loaded) {
    // Largura negativa no retângulo de origem espelha a textura — truque
    // padrão do raylib pra virar sprite por facing_right sem 2ª imagem.
    const float source_width =
        static_cast<float>(sprite->texture.width) * (fighter.facing_right ? 1.0f : -1.0f);
    const Rectangle source{0.0f, 0.0f, source_width, static_cast<float>(sprite->texture.height)};
    const Rectangle dest{fighter.position.x - kFighterHalfWidth, fighter.position.y - height,
                          kFighterHalfWidth * 2.0f, height};
    DrawTexturePro(sprite->texture, source, dest, Vector2{0.0f, 0.0f}, 0.0f, color);
    return;
  }

  DrawRectangle(static_cast<int>(fighter.position.x - kFighterHalfWidth),
                static_cast<int>(fighter.position.y - height),
                static_cast<int>(kFighterHalfWidth * 2.0f), static_cast<int>(height), color);
}

void DrawMatchOverlay(const Match& match) {
  constexpr int kBigFontSize = 64;
  constexpr int kSmallFontSize = 32;
  constexpr int kTitleY = 120;

  if (match.phase == RoundPhase::Intro) {
    const bool show_fight = match.phase_timer >= kIntroFrames / 2;
    const char* text = show_fight ? "FIGHT!" : TextFormat("ROUND %d", match.round_number);
    DrawCenteredText(text, kTitleY, kBigFontSize, RAYWHITE);
  }

  if (match.phase == RoundPhase::Fighting) {
    DrawCenteredText(TextFormat("%d", static_cast<int>(match.timer_seconds + 0.999f)), 20,
                      kSmallFontSize, RAYWHITE);
  }

  if (match.phase == RoundPhase::Ko) {
    DrawCenteredText(match.end_reason == RoundEndReason::Knockout ? "K.O.!" : "TIME UP", kTitleY,
                      kBigFontSize, RAYWHITE);
  }

  if (match.phase == RoundPhase::MatchOver) {
    DrawCenteredText(match.wins_p1 >= kWinsNeeded ? "P1 VENCEU A PARTIDA!"
                                                   : "P2 VENCEU A PARTIDA!",
                      kTitleY, kBigFontSize, RAYWHITE);
  }
}

void DrawHud(const Fighter& p1, const Fighter& p2, Color p1_color, Color p2_color) {
  const int p1_portrait_x = kHudMargin;
  const int p1_bar_x = p1_portrait_x + kHudPortraitSize + 10;
  const int p2_bar_x = kScreenWidth - kHudMargin - kHudPortraitSize - 10 - kHudBarWidth;
  const int p2_portrait_x = kScreenWidth - kHudMargin - kHudPortraitSize;
  const int meter_y = kHudBarY + kHudBarHeight + kHudMeterGap;
  const int combo_y = meter_y + kHudMeterHeight + 6;

  DrawRectangle(p1_portrait_x, kHudBarY, kHudPortraitSize, kHudPortraitSize, p1_color);
  DrawRectangleLines(p1_portrait_x, kHudBarY, kHudPortraitSize, kHudPortraitSize, RAYWHITE);
  DrawStatBar(p1_bar_x, kHudBarY, kHudBarHeight, static_cast<float>(p1.health) / 100.0f,
              Color{50, 15, 15, 255}, RED, /*anchor_right=*/false);
  DrawStatBar(p1_bar_x, meter_y, kHudMeterHeight, static_cast<float>(p1.super_meter) / 100.0f,
              Color{15, 15, 50, 255}, SKYBLUE, /*anchor_right=*/false);
  DrawComboText(p1_bar_x, combo_y, p1.combo_hits, /*anchor_right=*/false);

  DrawRectangle(p2_portrait_x, kHudBarY, kHudPortraitSize, kHudPortraitSize, p2_color);
  DrawRectangleLines(p2_portrait_x, kHudBarY, kHudPortraitSize, kHudPortraitSize, RAYWHITE);
  DrawStatBar(p2_bar_x, kHudBarY, kHudBarHeight, static_cast<float>(p2.health) / 100.0f,
              Color{50, 15, 15, 255}, RED, /*anchor_right=*/true);
  DrawStatBar(p2_bar_x, meter_y, kHudMeterHeight, static_cast<float>(p2.super_meter) / 100.0f,
              Color{15, 15, 50, 255}, SKYBLUE, /*anchor_right=*/true);
  DrawComboText(p2_bar_x, combo_y, p2.combo_hits, /*anchor_right=*/true);
}

void DrawProjectiles(const Match& match) {
  DrawProjectile(match.p1_projectile);
  DrawProjectile(match.p2_projectile);
}
