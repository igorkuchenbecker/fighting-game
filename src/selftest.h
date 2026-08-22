#pragma once

// Roda a simulação 3600 ticks com inputs scriptados por um LCG de seed
// fixa, duas vezes, e compara o hash do estado final — prova de
// determinismo (requisito duro: mesma seed, mesmo resultado sempre).
// Não abre janela (headless). Retorna 0 se os hashes baterem, 1 se não
// baterem. Uso: `./fighting_game --selftest`.
int RunSelfTest();
