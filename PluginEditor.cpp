/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
RaumsimulationAudioProcessorEditor::RaumsimulationAudioProcessorEditor(RaumsimulationAudioProcessor& p, juce::AudioProcessorValueTreeState& pts)
    : AudioProcessorEditor(&p)
    , openGLComponent(p, pts)
    , impulseResponseComponent(p, pts)
    , audioProcessor(p)
    , parameterTreeState(pts)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (1200, 900);
    setResizable(true, false);

    addAndMakeVisible(openGLComponent);
    addAndMakeVisible(impulseResponseComponent);

    addAndMakeVisible(gainSlider);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, gainSlider.getTextBoxWidth(), gainSlider.getTextBoxHeight());
    gainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    gainSlider.onValueChange = [this] { audioProcessor.updateParameters(); };

    gainLabel.attachToComponent(&gainSlider, false);
    gainAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(parameterTreeState, "gain", gainSlider));
}

RaumsimulationAudioProcessorEditor::~RaumsimulationAudioProcessorEditor()
{
}

//==============================================================================
void RaumsimulationAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void RaumsimulationAudioProcessorEditor::resized()
{
    auto height = getHeight();
    auto width = getWidth();

    openGLComponent.setBounds(0, 0, width/2, height/2);
    impulseResponseComponent.setBounds(0, 5*height/6, width/2, height/6);
    gainSlider.setBounds(width - 75, height - 75, 75, 75);
}
