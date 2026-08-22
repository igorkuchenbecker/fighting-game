#pragma once

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

// Dimensões/limites da arena — usados pela simulação (colisão, caixas de
// combate, em fighter.h/combat.h) e pelo render (DrawArena aqui).
constexpr float kArenaLeft = 100.0f;
constexpr float kArenaRight = 1180.0f;
constexpr float kFloorY = 600.0f;

void DrawArena();
