#include "../Source/ThreeKnobDSP.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <random>
#include <vector>
#include <limits>

using threeknob::Engine;

static int checks = 0;
void check(bool condition, const char* description) {
    ++checks;
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", description); std::exit(1); }
    std::printf("PASS: %s\n", description);
}

float rms(const std::vector<float>& v) {
    double energy = 0;
    for (float x : v) energy += static_cast<double>(x) * x;
    return static_cast<float>(std::sqrt(energy / static_cast<double>(v.size())));
}

int main() {
    constexpr int n = 8192;
    std::vector<float> left(n), right(n), original(n);
    auto resetTone = [&] {
        for (int i = 0; i < n; ++i) {
            const float t = static_cast<float>(i) / 48000.0f;
            left[i] = .5f * std::sin(2 * threeknob::pi * 210.0f * t)
                    + .2f * std::sin(2 * threeknob::pi * 3050.0f * t);
            right[i] = left[i] * .8f;
            original[i] = left[i];
        }
    };
    float* stereo[2] = { left.data(), right.data() };
    float* mono[1] = { left.data() };
    Engine engine;

    resetTone();
    engine.setParameters(0, 0, 0, false);
    engine.prepare(48000);
    engine.process(stereo, 2, n);
    bool exact = true;
    for (int i = 0; i < n; ++i) exact &= (left[i] == original[i]);
    check(exact, "All knobs at zero: bit-identical passthrough");

    resetTone();
    engine.setParameters(100, 100, 100, true);
    engine.prepare(48000);
    engine.process(stereo, 2, n);
    exact = true;
    for (int i = 0; i < n; ++i) exact &= left[i] == original[i];
    check(exact, "Bypass ON: bit-identical passthrough, even at max effect");

    resetTone();
    engine.setParameters(80, 70, 50, false);
    engine.prepare(48000);
    engine.process(stereo, 2, n);
    bool changed = false;
    bool finite = true;
    for (int i = 0; i < n; ++i) {
        changed |= std::fabs(left[i] - original[i]) > 1.e-4f;
        finite &= std::isfinite(left[i]) && std::isfinite(right[i]);
    }
    check(changed && finite, "Three controls color audio without NaN/Inf");

    // SUN works even with GOLD and GLOW off.
    resetTone();
    engine.setParameters(100, 0, 0, false);
    engine.prepare(48000);
    engine.process(mono, 1, n);
    changed = false;
    for (int i = 0; i < n; ++i) changed |= std::abs(left[i] - original[i]) > 1.e-4f;
    check(changed, "SUN independently alters the EQ contour");

    // GOLD works even with SUN and GLOW off.
    resetTone();
    engine.setParameters(0, 100, 0, false);
    engine.prepare(48000);
    engine.process(mono, 1, n);
    changed = false;
    for (int i = 0; i < n; ++i) changed |= std::abs(left[i] - original[i]) > 1.e-4f;
    check(changed, "GOLD independently adds nonlinear coloration");

    // GLOW does something transient-sensitive to repeated pulse-like events.
    std::fill(left.begin(), left.end(), 0.0f);
    for (int i = 0; i < n; ++i)
        left[i] = ((i % 1024) < 300) ? .6f * std::sin(2 * threeknob::pi * 700.0f * i / 48000.0f) : 0.f;
    original = left;
    engine.setParameters(0, 0, 100, false);
    engine.prepare(48000);
    engine.process(mono, 1, n);
    changed = false;
    for (int i = 0; i < n; ++i) changed |= std::abs(left[i] - original[i]) > 1.e-5f;
    check(changed, "GLOW produces transient-driven harmonic change");

    // Silence and invalid input should never poison the recursive filters.
    for (double sr : {8000.0, 11025.0, 44100.0, 48000.0, 96000.0, 192000.0, 384000.0}) {
        std::fill(left.begin(), left.end(), 0.f);
        std::fill(right.begin(), right.end(), 0.f);
        engine.setParameters(100, 100, 100, false);
        engine.prepare(sr);
        engine.process(stereo, 2, n);
        for (int i = 0; i < n; ++i) finite &= (left[i] == 0.f) && (right[i] == 0.f);
    }
    check(finite, "Silence stays silent at sample rates from 8 to 384 kHz");

    resetTone();
    engine.setParameters(100, 100, 100, false);
    engine.prepare(48000);
    left[100] = std::numeric_limits<float>::quiet_NaN();
    right[200] = std::numeric_limits<float>::infinity();
    engine.process(stereo, 2, n);
    finite = true;
    for (int i = 0; i < n; ++i) finite &= std::isfinite(left[i]) && std::isfinite(right[i]);
    check(finite, "Non-finite input is contained instead of poisoning the engine");

    resetTone();
    engine.setParameters(100, 100, 100, false);
    engine.prepare(48000);
    engine.process(stereo, 2, n);
    engine.setParameters(100, 100, 100, true);
    engine.process(stereo, 2, n);
    engine.process(stereo, 2, n);
    engine.process(stereo, 2, n);
    resetTone();
    engine.process(stereo, 2, n);
    exact = true;
    for (int i = 0; i < n; ++i) exact &= left[i] == original[i];
    check(exact, "Click-smoothed bypass settles to exact passthrough");

    // Vary block sizes and parameters; stress channel state using ASan+UBSan.
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> samples(-1.5f, 1.5f);
    std::uniform_real_distribution<float> params(0, 100);
    engine.prepare(44100);
    for (int round = 0; round < 1600; ++round) {
        const int block = 1 + round % n;
        for (int i = 0; i < block; ++i) {
            left[i] = samples(gen);
            right[i] = samples(gen);
        }
        engine.setParameters(params(gen), params(gen), params(gen), (round % 13 == 0));
        engine.process(stereo, (round & 1) ? 2 : 1, block);
        for (int i = 0; i < block; ++i) if (!std::isfinite(left[i])) finite = false;
    }
    check(finite, "Variable buffer sizes, mono/stereo, aggressive automation remain finite");

    engine.prepare(48000);
    engine.setParameters(100, 100, 100, false);
    auto before = std::chrono::steady_clock::now();
    for (int pass = 0; pass < 150; ++pass) {
        for (int i = 0; i < n; ++i) { left[i] = .2f; right[i] = .1f; }
        engine.process(stereo, 2, n);
    }
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - before).count();
    const auto soundSeconds = 150.0 * n / 48000.0;
    std::printf("BENCHMARK: %.1f seconds audio processed in %.3f seconds (%.1fx real-time)\n",
                soundSeconds, elapsed, soundSeconds / elapsed);
    check(elapsed < soundSeconds, "DSP faster than real-time on the test machine");

    std::printf("RESULT: %d checks passed.\n", checks);
    return 0;
}
