#include "stage.h"

#include "raylib.h"

void DrawArena() {
  DrawRectangle(0, static_cast<int>(kFloorY), kScreenWidth,
                kScreenHeight - static_cast<int>(kFloorY), DARKGRAY);
  DrawRectangle(static_cast<int>(kArenaLeft) - 10, 0, 10, static_cast<int>(kFloorY), GRAY);
  DrawRectangle(static_cast<int>(kArenaRight), 0, 10, static_cast<int>(kFloorY), GRAY);
}
