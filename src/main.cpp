#include "raylib.h"

#include <algorithm>
#include <cstdint>

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

constexpr float kArenaLeft = 100.0f;
constexpr float kArenaRight = 1180.0f;
constexpr float kFloorY = 600.0f;

constexpr float kFighterHalfWidth = 30.0f;
constexpr float kFighterStandHeight = 120.0f;
constexpr float kFighterCrouchHeight = 80.0f;

constexpr float kMoveSpeed = 6.0f;
constexpr float kJumpVelocity = -16.0f;
constexpr float kGravity = 0.8f;

// Direção quantizada em 8 vias: dado puro consumido pela simulação. A
// simulação nunca lê teclado diretamente (pré-requisito de replay/netcode).
enum class Direction8 : std::uint8_t {
  Neutral,
  Up,
  UpRight,
  Right,
  DownRight,
  Down,
  DownLeft,
  Left,
  UpLeft,
};

struct InputFrame {
  Direction8 direction = Direction8::Neutral;
  // Bitmask de botões de ataque; reservado, sem uso até a F3.
  std::uint8_t buttons = 0;
};

struct Fighter {
  Vector2 position{(kArenaLeft + kArenaRight) / 2.0f, kFloorY};
  Vector2 velocity{0.0f, 0.0f};
  bool is_grounded = true;
  bool is_crouching = false;
};

bool DirectionHasLeft(Direction8 direction) {
  return direction == Direction8::Left || direction == Direction8::UpLeft ||
         direction == Direction8::DownLeft;
}

bool DirectionHasRight(Direction8 direction) {
  return direction == Direction8::Right || direction == Direction8::UpRight ||
         direction == Direction8::DownRight;
}

bool DirectionHasUp(Direction8 direction) {
  return direction == Direction8::Up || direction == Direction8::UpLeft ||
         direction == Direction8::UpRight;
}

bool DirectionHasDown(Direction8 direction) {
  return direction == Direction8::Down || direction == Direction8::DownLeft ||
         direction == Direction8::DownRight;
}

// Único lugar que toca a API de input do raylib. Chamado uma vez por
// iteração do loop externo — nunca de dentro do step de simulação.
Direction8 ReadDirection8(int key_up, int key_down, int key_left, int key_right) {
  const bool up = IsKeyDown(key_up);
  const bool down = IsKeyDown(key_down) && !up;      // cima tem prioridade
  const bool left = IsKeyDown(key_left);
  const bool right = IsKeyDown(key_right) && !left;  // esquerda tem prioridade

  if (up && left) return Direction8::UpLeft;
  if (up && right) return Direction8::UpRight;
  if (down && left) return Direction8::DownLeft;
  if (down && right) return Direction8::DownRight;
  if (up) return Direction8::Up;
  if (down) return Direction8::Down;
  if (left) return Direction8::Left;
  if (right) return Direction8::Right;
  return Direction8::Neutral;
}

// Avança a simulação em um passo fixo de 1/60s. Puro e determinístico:
// só le o InputFrame recebido, nunca teclado/relógio/disco.
void StepFighter(Fighter& fighter, const InputFrame& input) {
  const bool wants_left = DirectionHasLeft(input.direction);
  const bool wants_right = DirectionHasRight(input.direction);
  const bool wants_up = DirectionHasUp(input.direction);
  const bool wants_down = DirectionHasDown(input.direction);

  fighter.is_crouching = fighter.is_grounded && wants_down;

  if (fighter.is_crouching) {
    fighter.velocity.x = 0.0f;
  } else if (wants_left && !wants_right) {
    fighter.velocity.x = -kMoveSpeed;
  } else if (wants_right && !wants_left) {
    fighter.velocity.x = kMoveSpeed;
  } else {
    fighter.velocity.x = 0.0f;
  }

  if (fighter.is_grounded && wants_up) {
    fighter.velocity.y = kJumpVelocity;
    fighter.is_grounded = false;
  }

  fighter.velocity.y += kGravity;

  fighter.position.x += fighter.velocity.x;
  fighter.position.y += fighter.velocity.y;

  if (fighter.position.y >= kFloorY) {
    fighter.position.y = kFloorY;
    fighter.velocity.y = 0.0f;
    fighter.is_grounded = true;
  }

  fighter.position.x = std::clamp(fighter.position.x, kArenaLeft + kFighterHalfWidth,
                                   kArenaRight - kFighterHalfWidth);
}

void DrawArena() {
  DrawRectangle(0, static_cast<int>(kFloorY), kScreenWidth,
                kScreenHeight - static_cast<int>(kFloorY), DARKGRAY);
  DrawRectangle(static_cast<int>(kArenaLeft) - 10, 0, 10, static_cast<int>(kFloorY), GRAY);
  DrawRectangle(static_cast<int>(kArenaRight), 0, 10, static_cast<int>(kFloorY), GRAY);
}

void DrawFighter(const Fighter& fighter) {
  const float height = fighter.is_crouching ? kFighterCrouchHeight : kFighterStandHeight;
  DrawRectangle(static_cast<int>(fighter.position.x - kFighterHalfWidth),
                static_cast<int>(fighter.position.y - height),
                static_cast<int>(kFighterHalfWidth * 2.0f), static_cast<int>(height), MAROON);
}

}  // namespace

int main() {
  InitWindow(kScreenWidth, kScreenHeight, "Fighting Game");
  SetTargetFPS(60);

  Fighter player;
  double accumulator = 0.0;

  while (!WindowShouldClose()) {
    const InputFrame input{ReadDirection8(KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT), 0};

    accumulator += GetFrameTime();
    // Simulação avança em passos fixos de 1/60s, desacoplada do delta-time
    // variável do render (pré-requisito para determinismo/rollback futuro).
    while (accumulator >= kFixedDt) {
      StepFighter(player, input);
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
