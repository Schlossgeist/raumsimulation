/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RaumsimulationAudioProcessorEditor::RaumsimulationAudioProcessorEditor(RaumsimulationAudioProcessor& p, juce::AudioProcessorValueTreeState& pts)
    : AudioProcessorEditor(&p), openGLComponent(pts), audioProcessor(p), parameterTreeState(pts)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (1200, 900);

    addAndMakeVisible(openGLComponent);
    setResizable(true, false);

    addAndMakeVisible(gainSlider);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, gainSlider.getTextBoxWidth(), gainSlider.getTextBoxHeight());
    gainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    gainLabel.attachToComponent(&gainSlider, false);
    gainAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment (parameterTreeState, "gain", gainSlider));
}

RaumsimulationAudioProcessorEditor::~RaumsimulationAudioProcessorEditor()
{
}

//==============================================================================
void RaumsimulationAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void RaumsimulationAudioProcessorEditor::resized()
{
    auto height = getHeight();
    auto width = getWidth();

    openGLComponent.setBounds(0, 0, width/2, height/2);
    gainSlider.setBounds(width - 75, height - 75, 75, 75);
}
