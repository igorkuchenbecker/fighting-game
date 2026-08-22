#include "training.h"

#include "raylib.h"

#include "combat.h"
#include "fighter.h"
#include "game.h"
#include "input.h"
#include "stage.h"

namespace {

constexpr Color kPlayerColor = MAROON;
constexpr Color kDummyColor = DARKBLUE;

void DrawBoxOutline(Rectangle rect, Color color) {
  DrawRectangleLines(static_cast<int>(rect.x), static_cast<int>(rect.y),
                      static_cast<int>(rect.width), static_cast<int>(rect.height), color);
}

// Hurtbox sempre (verde); hitbox só durante a fase Active (vermelho) —
// é quando o golpe realmente pode acertar.
void DrawDebugBoxes(const Fighter& player, const Fighter& dummy) {
  DrawBoxOutline(FighterHurtbox(player), GREEN);
  DrawBoxOutline(FighterHurtbox(dummy), GREEN);
  if (player.state == FighterState::Attack && player.attack_phase == AttackPhase::Active) {
    DrawBoxOutline(FighterHitbox(player, player.current_move), RED);
  }
  if (dummy.state == FighterState::Attack && dummy.attack_phase == AttackPhase::Active) {
    DrawBoxOutline(FighterHitbox(dummy, dummy.current_move), RED);
  }
}

const char* AttackPhaseName(AttackPhase phase) {
  switch (phase) {
    case AttackPhase::Startup:
      return "STARTUP";
    case AttackPhase::Active:
      return "ACTIVE";
    case AttackPhase::Recovery:
      return "RECOVERY";
    case AttackPhase::None:
      return "-";
  }
  return "-";  // inalcançável; silencia -Wreturn-type
}

void DrawFrameDataOverlay(const Fighter& player) {
  const MoveData& data = GetMoveData(player.current_move);
  DrawText(TextFormat("golpe=%d  startup=%d active=%d recovery=%d  |  fase=%s t=%d",
                       static_cast<int>(player.current_move), data.startup_frames,
                       data.active_frames, data.recovery_frames,
                       AttackPhaseName(player.attack_phase), player.state_timer),
           20, kScreenHeight - 40, 20, RAYWHITE);
}

}  // namespace

void RunTrainingMode() {
  Fighter player;
  Fighter dummy;
  ResetFighterForNewRound(player, kArenaLeft + 250.0f);
  ResetFighterForNewRound(dummy, kArenaRight - 250.0f);

  InputBuffer player_buffer;
  double accumulator = 0.0;
  bool show_debug_boxes = false;

  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_F1)) show_debug_boxes = !show_debug_boxes;

    // Mesmo mapa de P1 do modo normal (setas + espaço/enter/shift-direito,
    // ou gamepad 0). Único ponto que toca IsKeyDown/gamepad nesta função.
    player_buffer.Push(ReadInputFrame(KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_SPACE, KEY_ENTER,
                                       KEY_RIGHT_SHIFT, 0));

    accumulator += GetFrameTime();
    while (accumulator >= kFixedDt) {
      const InputFrame& player_input = player_buffer.AtDelay(0);
      const InputFrame dummy_input{};  // dummy nunca ataca; só apanha

      UpdateFacing(player, dummy);
      StepFighter(player, player_input);
      StepFighter(dummy, dummy_input);
      ResolveCombat(player, dummy, dummy_input);
      ResolveCombat(dummy, player, player_input);  // simétrico, sempre no-op (dummy não ataca)

      if (dummy.health <= 0) {
        ResetFighterForNewRound(dummy, dummy.position.x);  // dummy infinito: vida sempre volta
      }

      accumulator -= kFixedDt;
    }

    BeginDrawing();
    ClearBackground(BLACK);
    DrawArena();
    DrawFighter(player, kPlayerColor);
    DrawFighter(dummy, kDummyColor);
    if (show_debug_boxes) {
      DrawDebugBoxes(player, dummy);
    }
    DrawFrameDataOverlay(player);
    DrawText("MODO TREINO  —  F1: hit/hurtbox", 20, 20, 20, RAYWHITE);
    EndDrawing();
  }
}
