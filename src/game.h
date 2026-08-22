#pragma once

#include "combat.h"
#include "fighter.h"
#include "input.h"

// Timestep fixo da simulação — único lugar de verdade, usado tanto pelo
// loop externo (main.cpp) quanto pelo decremento do timer de round aqui.
constexpr double kFixedDt = 1.0 / 60.0;

enum class RoundPhase { Intro, Fighting, Ko, MatchOver };
enum class RoundEndReason { None, Knockout, Timeout };

constexpr int kWinsNeeded = 2;              // melhor de 3
constexpr float kRoundTimeSeconds = 99.0f;  // "timer 99s"
constexpr int kIntroFrames = 120;           // ~2s de "ROUND N" / "FIGHT!"
constexpr int kKoFrames = 90;               // ~1.5s de tela de KO antes do próximo round

struct Match {
  int round_number = 1;
  int wins_p1 = 0;
  int wins_p2 = 0;
  float timer_seconds = kRoundTimeSeconds;
  RoundPhase phase = RoundPhase::Intro;
  RoundEndReason end_reason = RoundEndReason::None;
  int phase_timer = 0;  // frames decorridos na fase atual
};

// Orquestra um passo de partida: durante RoundPhase::Fighting, avança a
// simulação dos dois lutadores (facing, física, combate) e o timer do
// round; nas demais fases, os lutadores ficam congelados e só o relógio
// de fase avança, até decidir o próximo passo (começar o round, fechar
// KO, encerrar a partida). Chamado 1x por tick fixo de 1/60s — o timer é
// decrementado por `kFixedDt` (constante, não relógio de parede), então
// continua determinístico.
void UpdateMatch(Match& match, Fighter& p1, Fighter& p2, const InputFrame& p1_input,
                  const InputFrame& p2_input);

void DrawMatchOverlay(const Match& match);
