#pragma once

// ============================================================================
// Sample rate audio globale.
// E' un constexpr (non un const qualsiasi) perche' viene usato anche per
// dimensionare A COMPILE-TIME i buffer di alcuni effetti (in particolare
// il Reverb, i cui filtri comb/allpass hanno lunghezze derivate da questo
// valore tramite template). Se lo cambi, tutti gli effetti si riadattano
// automaticamente ricompilando.
// ============================================================================
// constexpr float SAMPLE_RATE_HZ = 32000.0f;
