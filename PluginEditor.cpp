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
    , raytracer(p, pts, "Generating Impulse Response...", true, true, 1000, "Cancel", this->getParentComponent())
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

    addAndMakeVisible(generateIRButton);
    generateIRButton.setColour(TextButton::buttonColourId, Colour (0xff797fed));
    generateIRButton.setColour(TextButton::textColourOffId, Colours::black);
    generateIRButton.onClick = [this] { raytracer.launchThread(Thread::Priority::high); };

    addAndMakeVisible(generateLSButton);
    generateLSButton.setColour(TextButton::buttonColourId, Colour (0xff797fed));
    generateLSButton.setColour(TextButton::textColourOffId, Colours::black);
    generateLSButton.onClick = [this] { audioProcessor.ir.makeCopyOf(audioProcessor.generateLogarithmicSweep(20.0f, 20000.0f, 1.0f, audioProcessor.globalSampleRate, 2));
                                               impulseResponseComponent.updateThumbnail(audioProcessor.globalSampleRate); };
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

    openGLComponent.            setBounds(0, 0*height/6, width/2, 3*height/6);
    generateIRButton.           setBounds(0, 3*height/6, width/2, 1*height/6);
    generateLSButton.           setBounds(0, 4*height/6, width/2, 1*height/6);
    impulseResponseComponent.   setBounds(0, 5*height/6, width/2, 1*height/6);
    gainSlider.setBounds(width - 75, height - 75, 75, 75);
}
