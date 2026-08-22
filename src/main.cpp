#include "raylib.h"

#include "fighter.h"
#include "input.h"

#if defined(__SANITIZE_ADDRESS__)
// Driver proprietário NVIDIA (libnvidia-glcore/glsi) mantém alocações
// internas nunca liberadas por design; não são leaks do nosso código.
extern "C" const char* __lsan_default_suppressions() {
  return "leak:libnvidia-glcore.so\n"
         "leak:libnvidia-glsi.so\n";
}
#endif

namespace {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;
constexpr double kFixedDt = 1.0 / 60.0;

constexpr float kFighterStandHeight = 120.0f;
constexpr float kFighterCrouchHeight = 80.0f;

void DrawArena() {
  DrawRectangle(0, static_cast<int>(kFloorY), kScreenWidth,
                kScreenHeight - static_cast<int>(kFloorY), DARKGRAY);
  DrawRectangle(static_cast<int>(kArenaLeft) - 10, 0, 10, static_cast<int>(kFloorY), GRAY);
  DrawRectangle(static_cast<int>(kArenaRight), 0, 10, static_cast<int>(kFloorY), GRAY);
}

Color FighterColor(const Fighter& fighter) {
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
  return MAROON;
}

void DrawFighter(const Fighter& fighter) {
  const float height =
      fighter.state == FighterState::Crouch ? kFighterCrouchHeight : kFighterStandHeight;
  DrawRectangle(static_cast<int>(fighter.position.x - kFighterHalfWidth),
                static_cast<int>(fighter.position.y - height),
                static_cast<int>(kFighterHalfWidth * 2.0f), static_cast<int>(height),
                FighterColor(fighter));
}

}  // namespace

int main() {
  InitWindow(kScreenWidth, kScreenHeight, "Fighting Game");
  SetTargetFPS(60);

  Fighter player;
  InputBuffer input_buffer;
  double accumulator = 0.0;

  while (!WindowShouldClose()) {
    input_buffer.Push(ReadInputFrame(KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_SPACE));

    accumulator += GetFrameTime();
    // Simulação avança em passos fixos de 1/60s, desacoplada do delta-time
    // variável do render (pré-requisito para determinismo/rollback futuro).
    while (accumulator >= kFixedDt) {
      StepFighter(player, input_buffer.AtDelay(0));
      accumulator -= kFixedDt;
    }

    BeginDrawing();
    ClearBackground(BLACK);
    DrawArena();
    DrawFighter(player);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
