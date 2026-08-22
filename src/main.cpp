#include "raylib.h"

#include "combat.h"
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

void DrawArena() {
  DrawRectangle(0, static_cast<int>(kFloorY), kScreenWidth,
                kScreenHeight - static_cast<int>(kFloorY), DARKGRAY);
  DrawRectangle(static_cast<int>(kArenaLeft) - 10, 0, 10, static_cast<int>(kFloorY), GRAY);
  DrawRectangle(static_cast<int>(kArenaRight), 0, 10, static_cast<int>(kFloorY), GRAY);
}

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
  return base_color;
}

void DrawFighter(const Fighter& fighter, Color base_color) {
  const float height =
      fighter.state == FighterState::Crouch ? kFighterCrouchHeight : kFighterStandHeight;
  DrawRectangle(static_cast<int>(fighter.position.x - kFighterHalfWidth),
                static_cast<int>(fighter.position.y - height),
                static_cast<int>(kFighterHalfWidth * 2.0f), static_cast<int>(height),
                FighterColor(fighter, base_color));
}

}  // namespace

int main() {
  InitWindow(kScreenWidth, kScreenHeight, "Fighting Game");
  SetTargetFPS(60);

  Fighter player;
  Fighter dummy;
  dummy.position.x = kArenaRight - 200.0f;

  InputBuffer input_buffer;
  const InputFrame kDummyInput{};  // dummy parado: P2 jogável só chega na F4
  double accumulator = 0.0;

  while (!WindowShouldClose()) {
    input_buffer.Push(ReadInputFrame(KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_SPACE));

    accumulator += GetFrameTime();
    // Simulação avança em passos fixos de 1/60s, desacoplada do delta-time
    // variável do render (pré-requisito para determinismo/rollback futuro).
    while (accumulator >= kFixedDt) {
      const InputFrame& player_input = input_buffer.AtDelay(0);
      StepFighter(player, player_input);
      StepFighter(dummy, kDummyInput);
      // Resolvido nos dois sentidos: hoje só o player ataca, mas a forma
      // já é a correta para quando a F4 trouxer um P2 jogável de verdade.
      ResolveCombat(player, dummy, kDummyInput);
      ResolveCombat(dummy, player, player_input);
      accumulator -= kFixedDt;
    }

    BeginDrawing();
    ClearBackground(BLACK);
    DrawArena();
    DrawFighter(player, MAROON);
    DrawFighter(dummy, DARKBLUE);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
