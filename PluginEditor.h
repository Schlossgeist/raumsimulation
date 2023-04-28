/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DecibelSlider.h"
#include "PluginProcessor.h"
#include "OpenGLComponent.h"
#include "ImpulseResponseComponent.h"

//==============================================================================
/**
*/
class RaumsimulationAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    RaumsimulationAudioProcessorEditor(RaumsimulationAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~RaumsimulationAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    RaumsimulationAudioProcessor& audioProcessor;

    juce::AudioProcessorValueTreeState& parameterTreeState;

    juce::Label gainLabel{{}, "Gain"};
    DecibelSlider gainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    OpenGLComponent openGLComponent;
    ImpulseResponseComponent impulseResponseComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RaumsimulationAudioProcessorEditor)
};
