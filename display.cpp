#include "display.h"

// A string between two pegs. Each note sends a small wave out both ways from
// the Pick position; the waves bounce off the pegs, inverting as on a real
// string, and fade faster the more the string is damped. Bright sets their
// height, and Detune spreads faint companion strings above and below. Display
// code only: no audio state.
namespace pluck {
namespace {
using opxy::i32;
using opxy::u32;
using namespace opxy::draw;
const i32 kLeft = 120, kRight = 360, kY = 116;  // the string's ends and height, pixels
const i32 kLength = kRight - kLeft;            // 240 pixels: two outlines of 120
const i32 kSpeed = 260;                        // the waves' speed, pixels per second
const i32 kWave = 14;                          // half a wave's width, pixels
const u32 kPeg = 2, kMarker = 4, kCompanion = 5, kCompanions = 4;

// A control (Q31 0..1) as 0..1024.
i32 fraction(i32 control) { return (control < 0 ? 0 : control >> 16) * 1024 / 32767; }

// One wave: a single sine cycle across 2 * kWave pixels around its centre.
// d is the distance from the centre in Q10 pixels; the result is Q10, -1..1.
i32 wave(i32 d) {
    if (d <= -kWave * unit || d >= kWave * unit) return 0;
    return sine(d / kWave * 1800 / unit);
}

// Where a wave that set out from `start` has reached after moving `travel`
// (both Q10 pixels, signed), reflecting at both pegs; `sign` turns negative
// after an odd number of bounces.
i32 bounce(i32 start, i32 travel, i32& sign) {
    const i32 span = kLength * unit, reached = start + travel;
    const i32 laps = reached >= 0 ? reached / span : -((span - 1 - reached) / span);
    const i32 into = reached - laps * span;  // 0..span within this lap
    sign = laps & 1 ? -1 : 1;
    return laps & 1 ? span - into : into;
}
}

inline void display_reset(DisplayState& state) {
    state.amplitude = state.time = 0;
    state.serial = state.track = state.initialized = 0;
    state.key = 1;
}

inline void display(DisplayState& state, const Input& input, Frame& frame) {
    const i32 elapsed = clamp(input.elapsed, 0, 205);  // at most 0.2 s per frame
    const i32 bright = fraction(input.parameters[0]), at = fraction(input.parameters[1]);
    const i32 damp = fraction(input.parameters[2]), detune = fraction(input.parameters[3]);
    if (!state.initialized || state.track != input.track || input.reduced_motion) {
        state.amplitude = 0;
    } else if (input.note_serial != state.serial) {
        state.amplitude = (4 + bright * 16 / 1024) * unit;  // 4 to 20 pixels high
        state.time = 0;
    }
    state.initialized = 1;
    state.track = input.track;
    state.serial = input.note_serial;
    // Settle at 1.5 (little Damp) to 6 (full Damp) per second.
    const i32 rate = 1536 + damp * 4608 / 1024;
    const i32 settle = ((state.amplitude * rate) >> 10) * elapsed >> 10;
    state.amplitude -= settle > 0 ? settle : state.amplitude > 0 && elapsed > 0;
    if (state.amplitude < 0) state.amplitude = 0;
    state.time = clamp(state.time + elapsed, 0, 60 * unit);

    // The two waves, out from the pick point in each direction.
    const i32 pick = at * kLength;  // Q10 pixels from the left peg
    const i32 travel = state.time * kSpeed;
    i32 sign_right, sign_left;
    const i32 right = bounce(pick, travel, sign_right), left = bounce(pick, -travel, sign_left);
    // Each outline covers half the string, out along it and back between its
    // outgoing points: no point repeats, which the device's parser needs.
    for (u32 half = 0; half < 2; ++half) {
        Frame::path_type& path = frame.paths[half];
        for (u32 i = 0; i < 64; ++i) {
            const i32 u = i < 32 ? static_cast<i32>(i) * unit / 31 : static_cast<i32>(2 * (63 - i) + 1) * unit / 66;
            const i32 x = static_cast<i32>(half) * (kLength / 2) * unit + u * (kLength / 2);  // from the left peg
            const i32 height = sign_right * wave(x - right) + sign_left * wave(x - left);
            path.points[i].x = u * (kLength / 2) - (kLength / 4) * unit;
            path.points[i].y = clamp(floor_q10(state.amplitude * height), -40 * unit, 40 * unit);
        }
        place(frame, half, (kLeft + kLength / 4 + static_cast<i32>(half) * (kLength / 2)) * unit, kY * unit, unit, 0,
              stroke);
    }
    // New geometry only while the string moves; at rest it stays as it is.
    if (state.amplitude) state.key = state.key % 0x7ffffffeu + 1;
    frame.paths[0].key = frame.paths[1].key = state.key;
    place(frame, kPeg, kLeft * unit, kY * unit, unit, 1, fill_and_stroke);
    place(frame, kPeg + 1, kRight * unit, kY * unit, unit, 1, fill_and_stroke);
    place(frame, kMarker, kLeft * unit + pick, (kY + 34) * unit, unit, 1, fill_and_stroke);
    // Companion strings, spread evenly by Detune, up to 6 pixels apart.
    for (u32 c = 0; c < kCompanions; ++c) {
        const i32 step = c < 2 ? static_cast<i32>(c) + 1 : 1 - static_cast<i32>(c);  // +1, +2, -1, -2
        if (detune)
            place(frame, kCompanion + c, (kLeft + kLength / 2) * unit, kY * unit + step * detune * 6, unit, 2, stroke);
        else
            hide(frame, kCompanion + c);
    }
}
}
