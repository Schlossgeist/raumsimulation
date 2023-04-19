/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OpenGLComponent.h"

//==============================================================================
/**
*/
class RaumsimulationAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    RaumsimulationAudioProcessorEditor (RaumsimulationAudioProcessor&);
    ~RaumsimulationAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    RaumsimulationAudioProcessor& audioProcessor;

    OpenGLComponent openGLComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RaumsimulationAudioProcessorEditor)
};
