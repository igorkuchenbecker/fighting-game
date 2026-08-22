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

}  // namespace

InputFrame ReadInputFrame(int key_up, int key_down, int key_left, int key_right, int key_attack) {
  InputFrame frame;
  frame.direction = ReadDirection8(key_up, key_down, key_left, key_right);
  frame.buttons = IsKeyDown(key_attack) ? static_cast<std::uint8_t>(kButtonLight)
                                         : static_cast<std::uint8_t>(0);
  return frame;
}
