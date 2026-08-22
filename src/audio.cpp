#include "audio.h"

namespace {

Sound LoadIfExists(const char* path, bool& loaded_flag) {
  Sound sound{};
  if (FileExists(path)) {
    sound = LoadSound(path);
    loaded_flag = sound.frameCount > 0;
  }
  return sound;
}

void UnloadIfLoaded(Sound& sound, bool& loaded_flag) {
  if (loaded_flag) {
    UnloadSound(sound);
    loaded_flag = false;
  }
}

}  // namespace

SoundBank LoadSoundBank() {
  SoundBank bank;
  bank.hit = LoadIfExists("assets/audio/hit.wav", bank.hit_loaded);
  bank.block = LoadIfExists("assets/audio/block.wav", bank.block_loaded);
  bank.jump = LoadIfExists("assets/audio/jump.wav", bank.jump_loaded);
  bank.ko = LoadIfExists("assets/audio/ko.wav", bank.ko_loaded);
  return bank;
}

void UnloadSoundBank(SoundBank& bank) {
  UnloadIfLoaded(bank.hit, bank.hit_loaded);
  UnloadIfLoaded(bank.block, bank.block_loaded);
  UnloadIfLoaded(bank.jump, bank.jump_loaded);
  UnloadIfLoaded(bank.ko, bank.ko_loaded);
}

void PlayHitSound(const SoundBank& bank) {
  if (bank.hit_loaded) PlaySound(bank.hit);
}

void PlayBlockSound(const SoundBank& bank) {
  if (bank.block_loaded) PlaySound(bank.block);
}

void PlayJumpSound(const SoundBank& bank) {
  if (bank.jump_loaded) PlaySound(bank.jump);
}

void PlayKoSound(const SoundBank& bank) {
  if (bank.ko_loaded) PlaySound(bank.ko);
}
