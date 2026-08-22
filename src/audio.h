#pragma once

#include "raylib.h"

// Efeitos sonoros opcionais — assets/audio/*.wav SE existir; silencioso
// se ausente (mesmo padrão fallback de sprites.h: nunca crasha, nunca
// avisa). Precisa de InitAudioDevice() já chamado antes de carregar.
struct SoundBank {
  Sound hit{};
  Sound block{};
  Sound jump{};
  Sound ko{};
  bool hit_loaded = false;
  bool block_loaded = false;
  bool jump_loaded = false;
  bool ko_loaded = false;
};

SoundBank LoadSoundBank();

// Precisa ser chamado ANTES de CloseAudioDevice() (mesmo motivo de
// UnloadCharacterSprite/CloseWindow em sprites.h). Seguro numa bank sem
// nenhum som carregado (não faz nada).
void UnloadSoundBank(SoundBank& bank);

void PlayHitSound(const SoundBank& bank);
void PlayBlockSound(const SoundBank& bank);
void PlayJumpSound(const SoundBank& bank);
void PlayKoSound(const SoundBank& bank);
