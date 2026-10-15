#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout ThreeKnobAudioProcessor::makeLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> layout;
    const auto range = juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f);
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "sun", 1 }, "SUN", range, 55.0f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "gold", 1 }, "GOLD", range, 35.0f));
    layout.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "glow", 1 }, "GLOW", range, 25.0f));
    layout.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID { "bypass", 1 }, "BYPASS", false));
    return { layout.begin(), layout.end() };
}

ThreeKnobAudioProcessor::ThreeKnobAudioProcessor()
    : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
                            .withInput("Input", juce::AudioChannelSet::stereo(), true)
                            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "THREE_KNOB_STATE", makeLayout()) {}

bool ThreeKnobAudioProcessor::isBusesLayoutSupported(const BusesLayout& layout) const {
    const auto input = layout.getMainInputChannelSet();
    const auto output = layout.getMainOutputChannelSet();
    return input == output && (input == juce::AudioChannelSet::mono()
                               || input == juce::AudioChannelSet::stereo());
}

void ThreeKnobAudioProcessor::prepareToPlay(double fs, int) {
    engine.setParameters(parameters.getRawParameterValue("sun")->load(),
                         parameters.getRawParameterValue("gold")->load(),
                         parameters.getRawParameterValue("glow")->load(),
                         parameters.getRawParameterValue("bypass")->load() >= 0.5f);
    engine.prepare(fs);
}

void ThreeKnobAudioProcessor::releaseResources() {}

void ThreeKnobAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    engine.setParameters(parameters.getRawParameterValue("sun")->load(),
                         parameters.getRawParameterValue("gold")->load(),
                         parameters.getRawParameterValue("glow")->load(),
                         parameters.getRawParameterValue("bypass")->load() >= 0.5f);
    engine.process(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
}

juce::AudioProcessorEditor* ThreeKnobAudioProcessor::createEditor() {
    return new ThreeKnobAudioProcessorEditor(*this);
}

void ThreeKnobAudioProcessor::getStateInformation(juce::MemoryBlock& dest) {
    const auto state = parameters.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary(*xml, dest);
}

void ThreeKnobAudioProcessor::setStateInformation(const void* data, int size) {
    if (auto xml = getXmlFromBinary(data, size)) {
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new ThreeKnobAudioProcessor();
}
