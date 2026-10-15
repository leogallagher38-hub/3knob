#include "PluginEditor.h"
#include <cmath>

namespace {

const auto ink = juce::Colour(0xff1b211f);
const auto cream = juce::Colour(0xffefdfbc);
const auto muted = juce::Colour(0xffa6aa9b);
const auto brass = juce::Colour(0xffc8a56c);

}

void ThreeKnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                           float pos, float start, float end, juce::Slider&) {
    const float side = static_cast<float>(std::min(width, height));
    const auto cx = x + width * 0.5f;
    const auto cy = y + height * .5f;
    const float outer = side * .405f;
    g.setColour(juce::Colour(0xff111817));
    g.fillEllipse(cx - outer - 5.0f, cy - outer - 2.0f, (outer + 5.0f) * 2, (outer + 5.0f) * 2);
    g.setColour(juce::Colour(0xff4b4a3d));
    g.drawEllipse(cx - outer - 2, cy - outer - 2, (outer + 2) * 2, (outer + 2) * 2, 1.0f);

    juce::Path baseArc, activeArc;
    const float r = outer + 10.0f;
    baseArc.addCentredArc(cx, cy, r, r, 0, start, end, true);
    activeArc.addCentredArc(cx, cy, r, r, 0, start, start + pos * (end - start), true);
    g.setColour(juce::Colour(0xff444940));
    g.strokePath(baseArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(brass);
    g.strokePath(activeArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::ColourGradient metal(juce::Colour(0xfff4e9d4), cx - outer, cy - outer,
                               juce::Colour(0xffbcb19c), cx + outer, cy + outer, false);
    g.setGradientFill(metal);
    g.fillEllipse(cx - outer, cy - outer, outer * 2.0f, outer * 2.0f);
    g.setColour(juce::Colour(0xffa99b7d));
    g.drawEllipse(cx - outer, cy - outer, outer * 2.0f, outer * 2.0f, 1.4f);

    const float angle = start + pos * (end - start);
    const float p1 = outer * .42f, p2 = outer * .80f;
    const float sx = std::sin(angle), cyAngle = std::cos(angle);
    g.setColour(ink);
    g.drawLine(cx + sx * p1, cy - cyAngle * p1,
               cx + sx * p2, cy - cyAngle * p2, 3.5f);
    g.setColour(juce::Colour(0x55847b67));
    g.fillEllipse(cx - 2, cy - 2, 4, 4);
}

void ThreeKnobLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                                const juce::Colour&, bool over, bool down) {
    auto area = b.getLocalBounds().toFloat().reduced(1.0f);
    g.setColour(b.getToggleState() ? juce::Colour(0xffd1aa73) : juce::Colour(0xff303a36));
    g.fillRoundedRectangle(area, 8.0f);
    g.setColour(b.getToggleState() ? juce::Colour(0xffe5ca9b) : juce::Colour(0xff657068));
    g.drawRoundedRectangle(area, 8.0f, 1.0f);
    if (down || over) {
        g.setColour(juce::Colours::white.withAlpha(down ? .06f : .03f));
        g.fillRoundedRectangle(area, 8.0f);
    }
}

void ThreeKnobLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b, bool, bool) {
    g.setColour(b.getToggleState() ? ink : cream);
    g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
    g.drawFittedText(b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, 1);
}

ThreeKnobAudioProcessorEditor::ThreeKnobAudioProcessorEditor(ThreeKnobAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
    setLookAndFeel(&theme);
    auto init = [this](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 22);
        slider.setTextValueSuffix("");
        slider.setColour(juce::Slider::textBoxTextColourId, cream);
        slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        slider.setRotaryParameters(juce::degreesToRadians(225.0f),
                                   juce::degreesToRadians(495.0f), true);
        addAndMakeVisible(slider);
    };
    init(sun); init(gold); init(glow);
    bypass.setClickingTogglesState(true);
    bypass.setTooltip("Bypass 3 KNOB without losing the settings");
    addAndMakeVisible(bypass);
    sunLink = std::make_unique<SliderLink>(processor.parameters, "sun", sun);
    goldLink = std::make_unique<SliderLink>(processor.parameters, "gold", gold);
    glowLink = std::make_unique<SliderLink>(processor.parameters, "glow", glow);
    bypassLink = std::make_unique<ButtonLink>(processor.parameters, "bypass", bypass);
    setSize(530, 260);
}

ThreeKnobAudioProcessorEditor::~ThreeKnobAudioProcessorEditor() {
    bypassLink.reset(); glowLink.reset(); goldLink.reset(); sunLink.reset();
    setLookAndFeel(nullptr);
}

void ThreeKnobAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(ink);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff28322e), 0.0f, 0.0f,
                                           ink, 0.0f, static_cast<float>(getHeight()), false));
    g.fillRect(getLocalBounds());
    g.setColour(juce::Colour(0xff5e5948));
    g.drawRect(getLocalBounds().reduced(5), 1);
    g.setColour(cream);
    g.setFont(juce::Font(juce::FontOptions(30.0f, juce::Font::bold)));
    g.drawText("3 KNOB", 24, 13, 270, 42, juce::Justification::centredLeft);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(11.5f)));
    g.drawText("BY LEO GALLAGHER", 26, 53, 230, 19, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff655840));
    g.drawLine(26.0f, 76.0f, 505.0f, 76.0f, 1.0f);
    g.setColour(cream);
    g.setFont(juce::Font(juce::FontOptions(15.5f, juce::Font::bold)));
    g.drawText("SUN", 39, 209, 122, 22, juce::Justification::centred);
    g.drawText("GOLD", 205, 209, 122, 22, juce::Justification::centred);
    g.drawText("GLOW", 371, 209, 122, 22, juce::Justification::centred);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("TONE", 39, 232, 122, 15, juce::Justification::centred);
    g.drawText("HARMONICS", 205, 232, 122, 15, juce::Justification::centred);
    g.drawText("BLOOM", 371, 232, 122, 15, juce::Justification::centred);
}

void ThreeKnobAudioProcessorEditor::resized() {
    sun.setBounds(36, 81, 128, 127);
    gold.setBounds(202, 81, 128, 127);
    glow.setBounds(368, 81, 128, 127);
    bypass.setBounds(399, 25, 101, 34);
}
