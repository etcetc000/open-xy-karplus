#pragma once
#include <opxy/audio.hpp>

namespace pluck {
// The delay line holds one period of the string: 2048 samples reach about
// 21.5 Hz at 44.1 kHz, below the OP-XY's lowest note.
const unsigned kLength = 2048, kMask = kLength - 1;
struct Voice {
    opxy::i16 line[kLength];  // Q15 samples
    opxy::u32 write;          // next write position
    opxy::u32 seed;           // excitation noise
    opxy::i32 previous;       // the loop filter's last input
    opxy::u32 excited;        // 0 until the note's first block fills the line
};
// State shared by every voice; empty when voices share nothing.
struct Shared {};
}
