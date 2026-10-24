#include "core.h"
#include <opxy/q31.hpp>

// A plucked string (Karplus-Strong): a burst of filtered noise circulates in a
// delay line one period long, losing a little energy and treble every pass.
namespace pluck {
namespace {
using opxy::i32;
using opxy::u32;

// One period, in samples with 8 fractional bits, for a phase increment
// (2^32 per cycle). The loop filter adds half a sample, which is subtracted.
u32 period_q8(u32 increment) {
    const u32 longest = (kLength - 2) << 8;
    if (increment < 2) return longest;
    const u32 whole = 0xffffffffu / increment;
    if (whole >= kLength - 2) return longest;
    const u32 rest = 0xffffffffu - whole * increment;
    u32 fraction = rest / ((increment >> 8) + 1);
    if (fraction > 255) fraction = 255;
    const u32 period = (whole << 8) + fraction;
    return period > 640 ? period - 128 : 512;
}

i32 noise(u32& seed) {
    seed = seed * 1664525u + 1013904223u;
    return static_cast<i32>(seed >> 17) - 16384;  // Q15, half scale
}

i32 clamp15(i32 value) {
    return value > 16383 ? 16383 : value < -16384 ? -16384 : value;
}

// Q31 control 0..1 to a Q15 weight from low to high.
i32 weight(i32 control, i32 low, i32 high) {
    const i32 c = control < 0 ? 0 : control >> 16;
    return low + (high - low) * c / 32767;
}

// Fills one period with noise, low-passed by Bright and combed by Pick: a
// pick near the bridge keeps the upper partials, one near the middle removes
// the even ones.
void excite(Voice& voice, u32 period, i32 bright, i32 pick) {
    const u32 length = (period >> 8) + 1;
    const u32 offset = static_cast<u32>(weight(pick, 1638, 16384)) * length >> 15;
    i32 smooth = 0;
    for (u32 i = 0; i < length; ++i) {
        smooth += (noise(voice.seed) - smooth) * weight(bright, 2048, 32767) >> 15;
        voice.line[(voice.write - length + i) & kMask] = static_cast<opxy::i16>(smooth);
    }
    for (u32 i = length; i-- > offset;) {
        const u32 at = (voice.write - length + i) & kMask, before = (at - offset) & kMask;
        voice.line[at] = static_cast<opxy::i16>(clamp15(voice.line[at] - voice.line[before]));
    }
    voice.previous = 0;
}
}

// Callbacks run serially per voice; owner is a stable nonzero voice ID.
inline void initialize(Voice& voice, Shared&, u32 owner) {
    for (u32 i = 0; i < kLength; ++i) voice.line[i] = 0;
    voice.write = voice.previous = voice.excited = 0;
    voice.seed = owner * 2654435761u | 1u;
}
inline void start(Voice& voice, Shared&, u32) { voice.excited = 0; }
inline void destroy(Voice&, Shared&, u32) {}

// Controls: Decay, Bright, Pick, Level.
inline void render(Voice& voice, Shared&, u32, opxy::Output output, opxy::Parameters parameters,
                   opxy::Pitch pitch) {
    const u32 period = period_q8(pitch[0]);
    if (!voice.excited) {
        excite(voice, period, parameters[1], parameters[2]);
        voice.excited = 1;
    }
    // Loop gain per pass, 0.95 (a muted string) to 0.9997 (a long ring).
    const i32 gain = weight(parameters[0], 31130, 32758);
    const u32 delay = period >> 8, fraction = period & 255;
    for (unsigned i = 0; i < opxy::block_frames; ++i) {
        const i32 a = voice.line[(voice.write - delay) & kMask];
        const i32 b = voice.line[(voice.write - delay - 1) & kMask];
        const i32 sample = a + ((b - a) * static_cast<i32>(fraction) >> 8);
        // Two-point average: the string's gentle treble loss. Both steps round
        // toward zero, so the string dies away instead of settling at -1.
        const i32 filtered = (sample + voice.previous) / 2;
        voice.previous = sample;
        const i32 next = clamp15(filtered * gain / 32768);
        voice.line[voice.write & kMask] = static_cast<opxy::i16>(next);
        ++voice.write;
        output[i] = opxy::q31::multiply(next * 65536, parameters[3]);
    }
}
}
