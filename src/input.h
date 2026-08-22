#pragma once

#include <array>
#include <cstdint>

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

enum ButtonBit : std::uint8_t {
  kButtonLight = 1 << 0,
};

struct InputFrame {
  Direction8 direction = Direction8::Neutral;
  std::uint8_t buttons = 0;
};

bool DirectionHasLeft(Direction8 direction);
bool DirectionHasRight(Direction8 direction);
bool DirectionHasUp(Direction8 direction);
bool DirectionHasDown(Direction8 direction);

// Único ponto que toca a API de input do raylib (teclado). Chamado uma vez
// por iteração do loop externo — nunca de dentro do step de simulação.
InputFrame ReadInputFrame(int key_up, int key_down, int key_left, int key_right, int key_attack);

// Buffer circular de InputFrame por jogador (pré-requisito de netcode:
// permite consultar o histórico recente sem reler input do passado).
class InputBuffer {
 public:
  static constexpr int kCapacity = 10;

  void Push(InputFrame frame) {
    head_ = (head_ + 1) % kCapacity;
    frames_[static_cast<std::size_t>(head_)] = frame;
  }

  // frames_ago = 0 -> frame mais recente, 1 -> anterior, etc.
  [[nodiscard]] const InputFrame& AtDelay(int frames_ago) const {
    const int idx = ((head_ - frames_ago) % kCapacity + kCapacity) % kCapacity;
    return frames_[static_cast<std::size_t>(idx)];
  }

 private:
  std::array<InputFrame, kCapacity> frames_{};
  int head_ = 0;
};
