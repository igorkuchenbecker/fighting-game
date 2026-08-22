#include "input.h"

#include "raylib.h"

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

namespace {

constexpr float kGamepadDeadzone = 0.4f;

Direction8 CombineDirection(bool up, bool down, bool left, bool right) {
  // cima/esquerda têm prioridade sobre baixo/direita quando os dois lados
  // de um eixo vêm pressionados ao mesmo tempo.
  down = down && !up;
  right = right && !left;

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

Direction8 ReadDirection8Keyboard(int key_up, int key_down, int key_left, int key_right) {
  return CombineDirection(IsKeyDown(key_up), IsKeyDown(key_down), IsKeyDown(key_left),
                           IsKeyDown(key_right));
}

Direction8 ReadDirection8Gamepad(int gamepad) {
  if (!IsGamepadAvailable(gamepad)) return Direction8::Neutral;

  const float axis_x = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_X);
  const float axis_y = GetGamepadAxisMovement(gamepad, GAMEPAD_AXIS_LEFT_Y);

  const bool up =
      IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_UP) || axis_y < -kGamepadDeadzone;
  const bool down =
      IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_DOWN) || axis_y > kGamepadDeadzone;
  const bool left =
      IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || axis_x < -kGamepadDeadzone;
  const bool right =
      IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) || axis_x > kGamepadDeadzone;

  return CombineDirection(up, down, left, right);
}

}  // namespace

InputFrame ReadInputFrame(int key_up, int key_down, int key_left, int key_right, int key_light,
                           int key_medium, int key_heavy, int gamepad) {
  const Direction8 keyboard_direction = ReadDirection8Keyboard(key_up, key_down, key_left, key_right);
  const Direction8 gamepad_direction = ReadDirection8Gamepad(gamepad);

  InputFrame frame;
  frame.direction = gamepad_direction != Direction8::Neutral ? gamepad_direction : keyboard_direction;

  const bool gamepad_available = IsGamepadAvailable(gamepad);
  const bool light_down = IsKeyDown(key_light) ||
      (gamepad_available && IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_DOWN));
  const bool medium_down = IsKeyDown(key_medium) ||
      (gamepad_available && IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT));
  const bool heavy_down = IsKeyDown(key_heavy) ||
      (gamepad_available && IsGamepadButtonDown(gamepad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT));

  frame.buttons = static_cast<std::uint8_t>((light_down ? kButtonLight : 0) |
                                             (medium_down ? kButtonMedium : 0) |
                                             (heavy_down ? kButtonHeavy : 0));
  return frame;
}
