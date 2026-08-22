#include "game.h"

#include "raylib.h"

namespace {

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
}

void DrawCenteredText(const char* text, int y, int font_size, Color color) {
  const int width = MeasureText(text, font_size);
  DrawText(text, (kScreenWidth - width) / 2, y, font_size, color);
}

}  // namespace

void UpdateMatch(Match& match, Fighter& p1, Fighter& p2, const InputFrame& p1_input,
                  const InputFrame& p2_input) {
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
      StepFighter(p1, p1_input);
      StepFighter(p2, p2_input);
      // Resolvido nos dois sentidos: qualquer jogador pode ser o atacante.
      ResolveCombat(p1, p2, p2_input);
      ResolveCombat(p2, p1, p1_input);

      match.timer_seconds -= static_cast<float>(kFixedDt);

      if (p1.health <= 0 || p2.health <= 0) {
        match.end_reason = RoundEndReason::Knockout;
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
