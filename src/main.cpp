#include "raylib.h"

#include <cstring>

#include "audio.h"
#include "fighter.h"
#include "game.h"
#include "input.h"
#include "selftest.h"
#include "sprites.h"
#include "stage.h"
#include "training.h"

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

}  // namespace

int main(int argc, char** argv) {
  bool training_mode = false;
  bool training_ai_dummy = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--selftest") == 0) {
      return RunSelfTest();  // headless: sem InitWindow, só a simulação
    }
    if (std::strcmp(argv[i], "--training") == 0) {
      training_mode = true;
    }
    if (std::strcmp(argv[i], "--training-ai") == 0) {
      training_mode = true;
      training_ai_dummy = true;
    }
  }

  InitWindow(kScreenWidth, kScreenHeight, "Fighting Game");
  SetTargetFPS(60);

  if (training_mode) {
    RunTrainingMode(training_ai_dummy);
    CloseWindow();
    return 0;
  }

  Fighter p1;
  Fighter p2;
  p2.character = CharacterId::Gunner;  // P1 = Warrior (padrão), P2 = Gunner (tem projétil)
  ResetFighterForNewRound(p1, kArenaLeft + 250.0f);
  ResetFighterForNewRound(p2, kArenaRight - 250.0f);
  Match match;

  // Carrega sprite SE existir em assets/; senão fica com o retângulo de
  // sempre (fallback automático, ver DrawFighter/game.cpp).
  CharacterSprite p1_sprite = LoadCharacterSprite(p1.character);
  CharacterSprite p2_sprite = LoadCharacterSprite(p2.character);

  // Efeitos sonoros SE existirem em assets/audio/; silencioso se não
  // (mesmo padrão fallback das sprites).
  InitAudioDevice();
  SoundBank sound_bank = LoadSoundBank();

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
      const MatchEvents events = UpdateMatch(match, p1, p2, p1_buffer.AtDelay(0), p2_buffer.AtDelay(0));
      if (events.p1_hit_landed || events.p2_hit_landed) PlayHitSound(sound_bank);
      if (events.p1_hit_blocked || events.p2_hit_blocked) PlayBlockSound(sound_bank);
      if (events.p1_jumped || events.p2_jumped) PlayJumpSound(sound_bank);
      if (events.knockout_happened) PlayKoSound(sound_bank);
      accumulator -= kFixedDt;
    }

    BeginDrawing();
    ClearBackground(BLACK);
    DrawArena();
    DrawFighter(p1, kP1Color, &p1_sprite);
    DrawFighter(p2, kP2Color, &p2_sprite);
    DrawProjectiles(match);
    DrawMatchOverlay(match);
    DrawHud(p1, p2, kP1Color, kP2Color);
    EndDrawing();
  }

  UnloadSoundBank(sound_bank);
  CloseAudioDevice();
  UnloadCharacterSprite(p1_sprite);
  UnloadCharacterSprite(p2_sprite);
  CloseWindow();
  return 0;
}
