#include "sprites.h"

namespace {

const char* SpritePath(CharacterId character) {
  switch (character) {
    case CharacterId::Warrior:
      return "assets/warrior.png";
    case CharacterId::Gunner:
      return "assets/gunner.png";
  }
  return "";  // inalcançável; silencia -Wreturn-type
}

}  // namespace

CharacterSprite LoadCharacterSprite(CharacterId character) {
  CharacterSprite sprite;
  const char* path = SpritePath(character);
  if (FileExists(path)) {
    sprite.texture = LoadTexture(path);
    sprite.loaded = sprite.texture.id != 0;
  }
  return sprite;
}

void UnloadCharacterSprite(CharacterSprite& sprite) {
  if (sprite.loaded) {
    UnloadTexture(sprite.texture);
    sprite.loaded = false;
  }
}
