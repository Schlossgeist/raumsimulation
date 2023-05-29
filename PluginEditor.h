/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "DecibelSlider.h"
#include "ImpulseResponseComponent.h"
#include "OpenGLComponent.h"
#include "PluginProcessor.h"
#include "Raytracer.h"
#include "SettingsWindow.h"
#include <JuceHeader.h>

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
    void addObject();
    void removeObject();
    void populateObjectMenu();
    void populateObjectProperties();
    void updateObjectProperties();

    RaumsimulationAudioProcessor& audioProcessor;

    juce::AudioProcessorValueTreeState& parameterTreeState;

    TooltipWindow tooltipWindow{this, 100};

    juce::Label gainLabel{{}, "Gain"};
    DecibelSlider gainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    OpenGLComponent openGLComponent;
    ImpulseResponseComponent impulseResponseComponent;
    SettingsWindow settingsWindow;

    TextButton generateIRButton = {"Generate Impulse Response", {}};
    TextButton generateLSButton = {"Generate Logarithmic Sweep", {}};

    ImageButton settingsButton{"Settings"};

    TextButton addObjectButton = {"Create", {}};
    TextButton removeObjectButton = {"Remove", {}};
    ComboBox objectMenu;

    juce::Label nameEditorLabel{{}, "Name"};
    TextEditor nameEditor;

    juce::Label activeToggleLabel{{}, "Active"};
    ToggleButton activeToggle;
    ComboBox typeMenu;

    juce::Label xPositionLabel{{}, "X Position"};
    Slider      xPositionSlider;
    juce::Label yPositionLabel{{}, "Y Position"};
    Slider      yPositionSlider;
    juce::Label zPositionLabel{{}, "Z Position"};
    Slider      zPositionSlider;

    juce::Label xRotationLabel{{}, "X Rotation"};
    Slider      xRotationSlider;
    juce::Label yRotationLabel{{}, "Y Rotation"};
    Slider      yRotationSlider;
    juce::Label zRotationLabel{{}, "Z Rotation"};
    Slider      zRotationSlider;

    Raytracer raytracer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RaumsimulationAudioProcessorEditor)
};
