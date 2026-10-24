// Sound checks for Pluck, compiled for the host and run by
//   python <open-xy>/sdk.py test --machine .
#include "../engine.cpp"
#include <cstdio>
#include <initializer_list>

namespace {
int failures = 0;
void check(const char* what, bool ok) {
    if (!ok) { ++failures; std::printf("FAIL %s\n", what); }
}

const unsigned kBlocks = 690;  // one second
// Renders one note for a second.
void play(pluck::Voice& voice, double hz, const opxy::i32* parameters, opxy::i32* out) {
    pluck::Shared shared;
    pluck::initialize(voice, shared, 1);
    pluck::start(voice, shared, 1);
    opxy::u32 pitch[opxy::block_frames];
    for (unsigned i = 0; i < opxy::block_frames; ++i) pitch[i] = static_cast<opxy::u32>(hz / 44100.0 * 4294967296.0);
    for (unsigned b = 0; b < kBlocks; ++b) pluck::render(voice, shared, 1, out + b * opxy::block_frames, parameters, pitch);
}

double energy(const opxy::i32* out, unsigned from, unsigned count) {
    double sum = 0;
    for (unsigned i = from; i < from + count; ++i) sum += static_cast<double>(out[i]) * out[i];
    return sum / count;
}

// The shortest lag between 0.5 ms and 50 ms where the signal matches itself
// almost as well as it does anywhere: one period, not a multiple of it.
double period(const opxy::i32* out, unsigned from) {
    static double score[2205];
    double best = 0;
    for (unsigned lag = 22; lag < 2205; ++lag) {
        double sum = 0;
        for (unsigned i = from; i < from + 4096; ++i) sum += static_cast<double>(out[i]) * out[i + lag];
        score[lag] = sum;
        if (sum > best) best = sum;
    }
    for (unsigned lag = 23; lag + 1 < 2205; ++lag)
        if (score[lag] >= 0.9 * best && score[lag] >= score[lag - 1] && score[lag] >= score[lag + 1]) {
            // Parabolic interpolation around the peak, for fractional periods.
            const double a = score[lag - 1], b = score[lag], c = score[lag + 1];
            const double d = a - 2 * b + c;
            return lag + (d ? 0.5 * (a - c) / d : 0);
        }
    return 0;
}

bool bounded(const opxy::i32* out) {
    for (unsigned i = 0; i < kBlocks * opxy::block_frames; ++i)
        if (out[i] < -1073741824 || out[i] > 1073741824) return false;
    return true;
}

bool one_string(const pluck::Voice& voice) {
    for (unsigned i = 0; i < pluck::kLength; ++i)
        if (voice.strings[0].line[i] != voice.strings[1].line[i]) return false;
    return true;
}
}

int main() {
    static pluck::Voice voice;
    static opxy::i32 out[kBlocks * opxy::block_frames];
    const opxy::i32 kHalf = 0x40000000, kFull = 0x7fffffff;
    // Bright, Pick, Damp, Detune.
    for (opxy::i32 damp : {0, kHalf, kFull}) {
        const opxy::i32 parameters[8] = {kHalf, kHalf / 2, damp, 0};
        for (double hz : {110.0, 220.0, 440.0, 880.0}) {
            play(voice, hz, parameters, out);
            const double found = period(out, 2048), expected = 44100.0 / hz;
            check("in tune within 1% at every Damp", found > expected * 0.99 && found < expected * 1.01);
            check("rings on, then fades", energy(out, 30000, 4096) < energy(out, 1024, 4096) &&
                                          energy(out, 30000, 4096) > 0);
            check("within the declared peak", bounded(out));
        }
    }
    // Damp darkens: a tenth of a second in, the felt string's waveform changes
    // less from sample to sample, for its energy, than the glassy string's.
    auto roughness = [&](const opxy::i32* p) {
        play(voice, 220.0, p, out);
        double change = 0;
        for (unsigned i = 4410; i < 8506; ++i)
            change += static_cast<double>(out[i] - out[i - 1]) * (out[i] - out[i - 1]);
        return change / 4096 / energy(out, 4410, 4096);
    };
    const opxy::i32 glassy[8] = {kFull, kHalf / 4, 0, 0}, felt[8] = {kFull, kHalf / 4, kFull, 0};
    check("Damp darkens the ring", roughness(felt) < 0.5 * roughness(glassy));
    // Detune separates the two strings; without it they stay one.
    const opxy::i32 single[8] = {kHalf, kHalf / 2, kHalf / 4, 0}, doubled[8] = {kHalf, kHalf / 2, kHalf / 4, kFull};
    play(voice, 220.0, single, out);
    check("no Detune keeps one string", one_string(voice));
    play(voice, 220.0, doubled, out);
    check("Detune separates the strings", !one_string(voice));
    check("detuned within the declared peak", bounded(out));
    if (failures) std::printf("%d failures\n", failures);
    return failures ? 1 : 0;
}
