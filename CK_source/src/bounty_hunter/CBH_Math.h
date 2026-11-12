// CBH_Math.h - small random helpers of the game. Owner: src/bounty_hunter/CBH_Math.cpp (game2).
#ifndef CBH_MATH_H
#define CBH_MATH_H

class CBH_Math {
public:
    /** True with a chance of `probability` percent (clamped to 0..100). */
    static bool GetProbabiliyOutcome(int probability);
};

#endif
