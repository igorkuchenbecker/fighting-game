#pragma once

#include "raylib.h"

#include "fighter.h"

// Sprite estática por personagem — não uma spritesheet animada de
// verdade (sem arte real disponível ainda, ver docs/DECISOES.md).
// Carregada de assets/<nome>.png SE o arquivo existir; senão `loaded`
// fica false e quem desenha cai pro retângulo de sempre (fallback
// automático, sem crash, sem warning — requisito duro).
struct CharacterSprite {
  Texture2D texture{};
  bool loaded = false;
};

// Chamado 1x no início, com a janela/contexto GL já criado (precisa de
// InitWindow antes). Nunca falha nem avisa se o arquivo não existir.
CharacterSprite LoadCharacterSprite(CharacterId character);

// Precisa ser chamado ANTES de CloseWindow() (Texture2D é um recurso de
// GPU; descarregar depois do contexto morrer é comportamento indefinido).
// Seguro chamar em uma sprite não carregada (não faz nada).
void UnloadCharacterSprite(CharacterSprite& sprite);
