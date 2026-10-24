#pragma once
#include <opxy/draw.hpp>

namespace pluck {
// Matches resources.display in definition.json: paths, points, nodes, texts, palette.
// The string is two half-length outlines (paths 0 and 1), then its two pegs,
// the pick marker and four background strings for Detune.
using Frame = opxy::draw::Frame<2, 64, 9, 0, 3>;
struct DisplayState {
    opxy::i32 amplitude;     // the wave's height, Q10 pixels
    opxy::i32 time;          // since the pluck, Q10 seconds
    opxy::u32 serial;        // the last note start seen
    opxy::u32 track;
    opxy::u32 initialized;
    opxy::u32 key;           // the outlines' geometry key
};
}
