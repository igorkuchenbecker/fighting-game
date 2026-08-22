#include "raylib.h"

#include <cstdio>

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
}  // namespace

int main() {
  InitWindow(kScreenWidth, kScreenHeight, "Fighting Game");
  SetTargetFPS(60);

  double accumulator = 0.0;

  while (!WindowShouldClose()) {
    accumulator += GetFrameTime();
    // Simulação avança em passos fixos de 1/60s, desacoplada do delta-time
    // variável do render (pré-requisito para determinismo/rollback futuro).
    while (accumulator >= kFixedDt) {
      std::printf("tick\n");
      accumulator -= kFixedDt;
    }

    BeginDrawing();
    ClearBackground(BLACK);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
