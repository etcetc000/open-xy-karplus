#include "display.h"

// A string between two pegs. Each note plucks it at the Pick position; it
// swings and settles faster the more it is damped. Display code only: no audio state.
namespace pluck {
namespace {
using opxy::i32;
using opxy::u32;
using namespace opxy::draw;
const i32 kCentreX = 240, kCentreY = 116, kHalf = 60;  // pixels; the path is drawn at 2x
const i32 kPluck = 24 * unit;                            // swing just after a note, Q10 pixels

// Pick position along the string, Q10 0..1, from the Pick control (5% to 50%).
i32 pick(i32 control) {
    const i32 c = control < 0 ? 0 : control >> 16;
    return 51 + c * 461 / 32767;
}
}

inline void display_reset(DisplayState& state) {
    state.amplitude = 0;
    state.phase = state.serial = state.track = state.initialized = 0;
}

inline void display(DisplayState& state, const Input& input, Frame& frame) {
    const i32 elapsed = clamp(input.elapsed, 0, 205);  // at most 0.2 s per frame
    if (!state.initialized || state.track != input.track || input.reduced_motion) {
        state.amplitude = 0;
    } else if (input.note_serial != state.serial) {
        state.amplitude = kPluck;
    }
    state.initialized = 1;
    state.track = input.track;
    state.serial = input.note_serial;
    // Settle at 1.5 (little Damp) to 6 (full Damp) per second, and swing at 7 Hz.
    const i32 rate = 1536 + (input.parameters[2] >> 16) * 4608 / 32767;  // Q10 per second
    const i32 settle = ((state.amplitude * rate) >> 10) * elapsed >> 10;
    state.amplitude -= settle > 0 ? settle : state.amplitude > 0 && elapsed > 0;
    if (state.amplitude < 0) state.amplitude = 0;
    state.phase = (state.phase + static_cast<u32>(elapsed) * 7 * 3600 / 1024) % 3600;

    // The plucked shape is a triangle peaking at the pick, scaled by the swing.
    // The closed outline runs along the string and back.
    const i32 at = pick(input.parameters[1]);
    const i32 swing = state.amplitude * sine(static_cast<i32>(state.phase) + 900) >> 10;  // Q10 pixels
    Frame::path_type& path = frame.paths[0];
    // The way back samples between the outgoing points, so no two points of the
    // outline coincide: the device's SVG parser may merge repeated points, and
    // the renderer only reshapes an outline that keeps all of its points.
    for (u32 i = 0; i < 64; ++i) {
        const i32 u = i < 32 ? static_cast<i32>(i) * unit / 31 : static_cast<i32>(2 * (63 - i) + 1) * unit / 66;
        const i32 shape = u < at ? u * unit / at : (unit - u) * unit / (unit - at);
        Point& point = path.points[i];
        point.x = -kHalf * unit + 2 * kHalf * u;
        point.y = floor_q10(swing * shape);
    }
    path.key = 1 + static_cast<u32>(swing + 64 * unit) * 1024 + static_cast<u32>(at);
    place(frame, 0, kCentreX * unit, kCentreY * unit, 2 * unit, 0, stroke);
    place(frame, 1, (kCentreX - 2 * kHalf) * unit, kCentreY * unit, unit, 1, fill_and_stroke);
    place(frame, 2, (kCentreX + 2 * kHalf) * unit, kCentreY * unit, unit, 1, fill_and_stroke);
    place(frame, 3, (kCentreX - 2 * kHalf + 4 * kHalf * at / unit) * unit, (kCentreY + 34) * unit, unit, 1,
          fill_and_stroke);
}
}
