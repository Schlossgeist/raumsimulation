/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "CustomLookAndFeel.h"
#include "DecibelSlider.h"
#include "ImpulseResponseComponent.h"
#include "ObjectWindow.h"
#include "OpenGLComponent.h"
#include "PluginProcessor.h"
#include "Raytracer.h"
#include "SettingsWindow.h"
#include <JuceHeader.h>

//==============================================================================
/**
*/
class RaumsimulationAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    RaumsimulationAudioProcessorEditor(RaumsimulationAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~RaumsimulationAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    RaumsimulationAudioProcessor& audioProcessor;

    juce::AudioProcessorValueTreeState& parameterTreeState;

    CustomLookAndFeel customLookAndFeel;

    TooltipWindow tooltipWindow{this, 100};

    DecibelSlider gainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    OpenGLComponent openGLComponent;
    ImpulseResponseComponent impulseResponseComponent;
    SettingsWindow settingsWindow{audioProcessor, parameterTreeState, "Settings", Colours::black, DocumentWindow::TitleBarButtons::closeButton, true};
    ObjectWindow objectWindow{audioProcessor, parameterTreeState, raytracer, "Object", Colours::black, DocumentWindow::TitleBarButtons::closeButton, true};

    TextButton generateIRButton = {"Generate Impulse Response", {}};
    TextButton generateLSButton = {"Generate Logarithmic Sweep", {}};

    DrawableButton settingsButton{"Settings", juce::DrawableButton::ImageOnButtonBackground};
    DrawableButton addObjectButton{"Add object", juce::DrawableButton::ImageOnButtonBackground};
    DrawableButton editObjectButton{"Edit object", juce::DrawableButton::ImageOnButtonBackground};

    Raytracer raytracer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RaumsimulationAudioProcessorEditor)
};
