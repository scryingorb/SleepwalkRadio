#pragma once
#include <array>

struct FactoryPreset { const char* name; std::array<float, 8> values; };
// Host program 0 is a neutral initialization; the following twelve are factory voices.
// Warmth, Wobble, Flutter, Age, Hiss, Mix, Output dB, Bypass.
inline constexpr std::array<FactoryPreset, 13> factoryPresets {{
    { "Init",               { 0,  0,  0,  0,  0,100, 0,0} },
    { "Warm Memories",      {30, 12, 10, 18,  5,100, 0,0} },
    { "Midnight Radio",     {42, 18, 22, 68, 24,100, 1,0} },
    { "Melted Cassette",    {62, 94, 58, 38, 20,100, 0,0} },
    { "Dream Sequence",     {25, 45, 15, 42,  8, 85, 0,0} },
    { "Broken Walkman",     {72, 65, 85, 88, 34,100, 1,0} },
    { "VHS Sunset",         {55, 30, 36, 32, 14,100, 0,0} },
    { "Underwater",         {28, 32, 12,100,  0,100, 1,0} },
    { "Ghost Transmission", {18, 62, 24, 72, 38, 80,-2,0} },
    { "Old Photograph",     {20,  6,  6, 28,  4,100, 0,0} },
    { "Lost Signal",        {88,100,100, 96, 60,100, 0,0} },
    { "Analog Heaven",      {65, 24, 18, 14,  6,100, 0,0} },
    { "Sleepwalking",       {45, 54, 28, 48, 16, 92, 0,0} }
}};
