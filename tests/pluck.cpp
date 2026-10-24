// Sound checks for Pluck, compiled for the host (see README.md):
//   c++ -std=c++11 -I <open-xy>/sdk/include tests/pluck.cpp -o pluck-test && ./pluck-test
#include "../engine.cpp"
#include <cstdio>
#include <initializer_list>

namespace {
int failures = 0;
void check(const char* what, bool ok) {
    if (!ok) { ++failures; std::printf("FAIL %s\n", what); }
}

// Renders one note for `blocks` blocks into `out`.
void play(pluck::Voice& voice, double hz, const opxy::i32* parameters, opxy::i32* out, unsigned blocks) {
    pluck::Shared shared;
    pluck::initialize(voice, shared, 1);
    pluck::start(voice, shared, 1);
    opxy::u32 pitch[opxy::block_frames];
    for (unsigned i = 0; i < opxy::block_frames; ++i) pitch[i] = static_cast<opxy::u32>(hz / 44100.0 * 4294967296.0);
    for (unsigned b = 0; b < blocks; ++b) pluck::render(voice, shared, 1, out + b * opxy::block_frames, parameters, pitch);
}

double energy(const opxy::i32* out, unsigned from, unsigned count) {
    double sum = 0;
    for (unsigned i = from; i < from + count; ++i) sum += static_cast<double>(out[i]) * out[i];
    return sum / count;
}

// The shortest lag between 0.5 ms and 50 ms where the signal matches itself
// almost as well as it does anywhere: one period, not a multiple of it.
unsigned period(const opxy::i32* out, unsigned from) {
    static double score[2205];
    double best = 0;
    for (unsigned lag = 22; lag < 2205; ++lag) {
        double sum = 0;
        for (unsigned i = from; i < from + 4096; ++i) sum += static_cast<double>(out[i]) * out[i + lag];
        score[lag] = sum;
        if (sum > best) best = sum;
    }
    for (unsigned lag = 23; lag + 1 < 2205; ++lag)
        if (score[lag] >= 0.9 * best && score[lag] >= score[lag - 1] && score[lag] >= score[lag + 1]) return lag;
    return 0;
}
}

int main() {
    static pluck::Voice voice;
    static opxy::i32 out[690 * opxy::block_frames];  // one second
    const opxy::i32 nylon[8] = {1288490188, 966367641, 751619276, 1717986918};
    for (double hz : {110.0, 220.0, 440.0, 880.0}) {
        play(voice, hz, nylon, out, 690);
        const unsigned found = period(out, 2048);
        const double expected = 44100.0 / hz;
        check("tuned within 1%", found > expected * 0.99 - 1 && found < expected * 1.01 + 1);
        check("decays", energy(out, 30000, 4096) < energy(out, 1024, 4096));
        bool bounded = true;
        for (unsigned i = 0; i < 690 * opxy::block_frames; ++i) bounded = bounded && out[i] >= -1073741824 && out[i] <= 1073741824;
        check("within the declared peak", bounded);
    }
    const opxy::i32 muted[8] = {0, 966367641, 751619276, 1717986918};
    play(voice, 220.0, muted, out, 690);
    check("short decay is quiet after a second", energy(out, 40000, 4096) < 1e-6 * energy(out, 1024, 4096));
    if (failures) std::printf("%d failures\n", failures);
    return failures ? 1 : 0;
}
