#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ThreeKnobLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
};

class ThreeKnobAudioProcessorEditor final : public juce::AudioProcessorEditor {
public:
    explicit ThreeKnobAudioProcessorEditor(ThreeKnobAudioProcessor&);
    ~ThreeKnobAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    ThreeKnobAudioProcessor& processor;
    ThreeKnobLookAndFeel theme;
    juce::Slider sun, gold, glow;
    juce::TextButton bypass { "BYPASS" };
    using SliderLink = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonLink = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderLink> sunLink, goldLink, glowLink;
    std::unique_ptr<ButtonLink> bypassLink;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreeKnobAudioProcessorEditor)
};
