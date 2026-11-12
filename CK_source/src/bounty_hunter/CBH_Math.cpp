// CBH_Math.cpp - random helpers (CK 1.5.2, written by game2).
#include "CBH_Math.h"
#include "CRandGen.h"

bool CBH_Math::GetProbabiliyOutcome(int probability)
{
    if (probability < 0) {
        probability = 0;
    } else if (probability > 100) {
        probability = 100;
    }
    return CRandGen::GetInstance()->GetRandRange(0, 100) <= probability;
}
