#pragma once
#include <opxy/draw.hpp>

namespace pluck {
// Matches resources.display in definition.json: paths, points, nodes, texts, palette.
// The string is path 0, then its two pegs and the pick marker.
using Frame = opxy::draw::Frame<1, 64, 4, 0, 2>;
struct DisplayState {
    opxy::i32 amplitude;     // Q10 pixels, before the string's 2x scale
    opxy::u32 phase;         // tenths of a degree
    opxy::u32 serial;        // the last note start seen
    opxy::u32 track;
    opxy::u32 initialized;
};
}
