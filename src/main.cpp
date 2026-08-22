#include "raylib.h"

#include "fighter.h"
#include "game.h"
#include "input.h"
#include "stage.h"

#if defined(__SANITIZE_ADDRESS__)
// Driver proprietário NVIDIA (libnvidia-glcore/glsi) mantém alocações
// internas nunca liberadas por design; não são leaks do nosso código.
extern "C" const char* __lsan_default_suppressions() {
  return "leak:libnvidia-glcore.so\n"
         "leak:libnvidia-glsi.so\n";
}
#endif

namespace {

constexpr Color kP1Color = MAROON;
constexpr Color kP2Color = DARKBLUE;

// `base_color` distingue P1/P2 (dois tons, exigido enquanto o visual for
// retângulos de código nas F0-4); as fases do ataque sobrepõem essa cor.
Color FighterColor(const Fighter& fighter, Color base_color) {
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

void DrawFighter(const Fighter& fighter, Color base_color) {
  const float height = FighterDrawHeight(fighter);
  DrawRectangle(static_cast<int>(fighter.position.x - kFighterHalfWidth),
                static_cast<int>(fighter.position.y - height),
                static_cast<int>(kFighterHalfWidth * 2.0f), static_cast<int>(height),
                FighterColor(fighter, base_color));
}

}  // namespace

int main() {
  InitWindow(kScreenWidth, kScreenHeight, "Fighting Game");
  SetTargetFPS(60);

  Fighter p1;
  Fighter p2;
  p2.character = CharacterId::Gunner;  // P1 = Warrior (padrão), P2 = Gunner (tem projétil)
  ResetFighterForNewRound(p1, kArenaLeft + 250.0f);
  ResetFighterForNewRound(p2, kArenaRight - 250.0f);
  Match match;

  InputBuffer p1_buffer;
  InputBuffer p2_buffer;
  double accumulator = 0.0;

  while (!WindowShouldClose()) {
    // P1: setas + espaço/enter/shift-direito (leve/médio/pesado), ou
    // gamepad 0. P2: WASD + ctrl-esquerdo/shift-esquerdo/Q, ou gamepad 1.
    // Único ponto que toca IsKeyDown/gamepad (via ReadInputFrame).
    p1_buffer.Push(ReadInputFrame(KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_SPACE, KEY_ENTER,
                                   KEY_RIGHT_SHIFT, 0));
    p2_buffer.Push(ReadInputFrame(KEY_W, KEY_S, KEY_A, KEY_D, KEY_LEFT_CONTROL, KEY_LEFT_SHIFT,
                                   KEY_Q, 1));

    accumulator += GetFrameTime();
    // Simulação avança em passos fixos de 1/60s, desacoplada do delta-time
    // variável do render (pré-requisito para determinismo/rollback futuro).
    while (accumulator >= kFixedDt) {
      UpdateMatch(match, p1, p2, p1_buffer.AtDelay(0), p2_buffer.AtDelay(0));
      accumulator -= kFixedDt;
    }

    BeginDrawing();
    ClearBackground(BLACK);
    DrawArena();
    DrawFighter(p1, kP1Color);
    DrawFighter(p2, kP2Color);
    DrawProjectiles(match);
    DrawMatchOverlay(match);
    DrawHud(p1, p2, kP1Color, kP2Color);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
