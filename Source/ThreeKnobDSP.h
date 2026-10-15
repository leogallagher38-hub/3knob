#pragma once

// 3 KNOB by Leo Gallagher - small, dependency-free audio engine.
// All buffers and DSP state are allocated ahead of playback. No allocations in process().

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace threeknob {

constexpr float pi = 3.14159265358979323846f;

inline float limited(float x, float lo, float hi) noexcept {
    return std::isfinite(x) ? std::max(lo, std::min(x, hi)) : lo;
}

struct Biquad {
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void reset() noexcept { z1 = z2 = 0.0f; }

    inline float process(float x) noexcept {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void update(double B0, double B1, double B2, double A0, double A1, double A2) noexcept {
        // All caller-generated A0 values are strictly positive (RBJ filter designs).
        const double inv = 1.0 / A0;
        b0 = static_cast<float>(B0 * inv);
        b1 = static_cast<float>(B1 * inv);
        b2 = static_cast<float>(B2 * inv);
        a1 = static_cast<float>(A1 * inv);
        a2 = static_cast<float>(A2 * inv);
    }

    void bell(double hz, double q, double decibels, double fs) noexcept {
        const double w = 2.0 * static_cast<double>(pi) * hz / fs;
        const double c = std::cos(w);
        const double alpha = std::sin(w) / (2.0 * q);
        const double A = std::pow(10.0, decibels / 40.0);
        update(1.0 + alpha * A, -2.0 * c, 1.0 - alpha * A,
               1.0 + alpha / A, -2.0 * c, 1.0 - alpha / A);
    }

    void shelf(double hz, double decibels, double fs, bool high) noexcept {
        const double w = 2.0 * static_cast<double>(pi) * hz / fs;
        const double c = std::cos(w);
        const double A = std::pow(10.0, decibels / 40.0);
        const double sqrtA = std::sqrt(A);
        const double t = 2.0 * sqrtA * std::sin(w) / std::sqrt(2.0); // S = 1
        if (high) {
            update(A * ((A + 1) + (A - 1) * c + t),
                   -2 * A * ((A - 1) + (A + 1) * c),
                   A * ((A + 1) + (A - 1) * c - t),
                   (A + 1) - (A - 1) * c + t,
                   2 * ((A - 1) - (A + 1) * c),
                   (A + 1) - (A - 1) * c - t);
        } else {
            update(A * ((A + 1) - (A - 1) * c + t),
                   2 * A * ((A - 1) - (A + 1) * c),
                   A * ((A + 1) - (A - 1) * c - t),
                   (A + 1) + (A - 1) * c + t,
                   -2 * ((A - 1) + (A + 1) * c),
                   (A + 1) + (A - 1) * c - t);
        }
    }
};

struct OnePole {
    float z = 0.0f;
    float a = 0.0f;
    void setFrequency(float hz, double fs) noexcept {
        a = 1.0f - std::exp(-2.0f * pi * hz / static_cast<float>(fs));
    }
    float process(float x) noexcept { z += a * (x - z); return z; }
    void reset() noexcept { z = 0.0f; }
};

class Engine {
public:
    void setParameters(float sun, float gold, float glow, bool bypass) noexcept {
        targetSun = limited(sun, 0.0f, 100.0f) * .01f;
        targetGold = limited(gold, 0.0f, 100.0f) * .01f;
        targetGlow = limited(glow, 0.0f, 100.0f) * .01f;
        targetWet = bypass ? 0.0f : 1.0f;
    }

    void prepare(double newSampleRate) noexcept {
        sampleRate = std::max(8000.0, std::min(384000.0, newSampleRate));
        sun = targetSun;
        gold = targetGold;
        glow = targetGlow;
        wet = targetWet;
        goldSmoothing = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate * .014));
        glowSmoothing = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate * .014));
        bypassSmoothing = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate * .004));
        slowAttack = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate * .045));
        bloomAttack = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate * .022));
        bloomRelease = 1.0f - std::exp(-1.0f / static_cast<float>(sampleRate * .190));
        fastRelease = std::exp(-1.0f / static_cast<float>(sampleRate * .025));
        for (auto& c : channels) {
            for (auto& f : c.eq) f.reset();
            c.low.reset(); c.lowMid.reset();
            c.low.setFrequency(170.0f, sampleRate);
            c.lowMid.setFrequency(3100.0f, sampleRate);
            c.fast = c.slow = c.bloom = 0.0f;
        }
        updateEq();
    }

    // The host owns the writable, non-interleaved float buffers. Supports mono/stereo.
    void process(float* const* data, int channelCount, int sampleCount) noexcept {
        if (data == nullptr || channelCount < 1 || sampleCount <= 0) return;
        channelCount = std::min(channelCount, 2);

        // Coefficient refresh once per host block. Smooth in seconds, not blocks.
        const float motion = 1.0f - std::exp(-static_cast<float>(sampleCount / (sampleRate * .050)));
        const float nextSun = sun + (targetSun - sun) * motion;
        if (std::abs(nextSun - sun) > 0.000001f) {
            sun = nextSun;
            updateEq();
        }

        for (int i = 0; i < sampleCount; ++i) {
            gold += (targetGold - gold) * goldSmoothing;
            glow += (targetGlow - glow) * glowSmoothing;
            wet += (targetWet - wet) * bypassSmoothing;
            if (std::abs(targetWet - wet) < 1.e-7f) wet = targetWet;

            // Static compensation is deliberately modest: no pumping or extra latency.
            const float trim = 1.0f - (.055f * sun + .025f * gold + .010f * glow);
            for (int ch = 0; ch < channelCount; ++ch) {
                if (data[ch] == nullptr) continue;
                const float input = std::isfinite(data[ch][i]) ? data[ch][i] : 0.0f;
                auto& c = channels[static_cast<size_t>(ch)];
                float x = input;
                for (auto& f : c.eq) x = f.process(x);

                // Complementary parallel one-pole bands sum to the original x exactly
                // before nonlinearity, avoiding extra crossover phase shift on recombination.
                const float lo = c.low.process(x);
                const float lowMid = c.lowMid.process(x);
                const float mid = lowMid - lo;
                const float hi = x - lowMid;

                float colored = x;
                if (gold > 0.00001f) {
                    colored += (softSaturate(lo, 1.0f + 1.25f * gold, 0.02f * gold) - lo) * (.22f * gold);
                    colored += (softSaturate(mid, 1.0f + 2.5f * gold, 0.09f * gold) - mid) * (.66f * gold);
                    colored += (softSaturate(hi, 1.0f + .7f * gold, 0.0f) - hi) * (.12f * gold);
                }

                if (glow > 0.00001f) {
                    const float energy = std::abs(x);
                    c.fast = std::max(energy, c.fast * fastRelease);
                    c.slow += (energy - c.slow) * slowAttack;
                    const float onset = limited((c.fast - c.slow) / (c.fast + .06f), 0.0f, 1.0f);
                    c.bloom += (onset - c.bloom) * (onset > c.bloom ? bloomAttack : bloomRelease);
                    const float glowSource = mid + .28f * hi;
                    const float harmonic = softSaturate(glowSource, 3.2f + 1.6f * glow, .09f) - glowSource;
                    colored += harmonic * c.bloom * (.70f * glow);
                }
                const float effected = colored * trim;
                data[ch][i] = input + wet * (effected - input);
            }
        }
    }

private:
    // Tiny, stable saturator (one division, no tanh calls or lookahead).
    static float softSaturate(float x, float drive, float bias) noexcept {
        auto transfer = [](float v) noexcept { return v / (1.0f + .42f * std::abs(v)); };
        const float biasDerivativeInv = (1.0f + .42f * std::abs(bias));
        const float normalized = (transfer(x * drive + bias) - transfer(bias))
                               * (biasDerivativeInv * biasDerivativeInv / drive);
        return normalized;
    }

    void updateEq() noexcept {
        const double fs = sampleRate;
        const auto freq = [fs](double f) { return std::min(f, fs * 0.43); };
        for (auto& c : channels) {
            c.eq[0].shelf(freq(125.0), 1.7 * sun, fs, false);
            c.eq[1].bell(freq(365.0), .75, -1.2 * sun, fs);
            c.eq[2].bell(freq(2600.0), .9, 1.2 * sun, fs);
            c.eq[3].shelf(freq(8500.0), -.9 * sun, fs, true);
        }
    }

    struct Channel {
        std::array<Biquad, 4> eq;
        OnePole low, lowMid;
        float fast = 0.0f, slow = 0.0f, bloom = 0.0f;
    };

    std::array<Channel, 2> channels;
    double sampleRate = 48000.0;
    float targetSun = 0.0f, targetGold = 0.0f, targetGlow = 0.0f, targetWet = 1.0f;
    float sun = 0.0f, gold = 0.0f, glow = 0.0f, wet = 1.0f;
    float goldSmoothing = 0.0f, glowSmoothing = 0.0f, bypassSmoothing = 0.0f;
    float slowAttack = 0.0f, bloomAttack = 0.0f, bloomRelease = 0.0f, fastRelease = 0.0f;
};
} // namespace threeknob
