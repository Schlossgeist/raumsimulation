/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Raytracer.h"

//==============================================================================
RaumsimulationAudioProcessorEditor::RaumsimulationAudioProcessorEditor(RaumsimulationAudioProcessor& p, juce::AudioProcessorValueTreeState& pts)
    : AudioProcessorEditor(&p)
    , impulseResponseComponent(p, pts)
    , audioProcessor(p)
    , parameterTreeState(pts)
    , raytracer(p, pts, impulseResponseComponent, "Generating Impulse Response...", true, true, 1000, "Cancel", this->getParentComponent())
    , openGLComponent(p, pts, raytracer)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (1200, 900);
    setResizable(true, false);

    LookAndFeel::setDefaultLookAndFeel(&customLookAndFeel);

    addAndMakeVisible(openGLComponent);
    addAndMakeVisible(impulseResponseComponent);

    addAndMakeVisible(gainSlider);
    gainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, gainSlider.getTextBoxWidth(), gainSlider.getTextBoxHeight());
    gainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    gainSlider.onValueChange = [this] { audioProcessor.updateParameters(); };
    gainAttachment.reset(new juce::AudioProcessorValueTreeState::SliderAttachment(parameterTreeState, "gain", gainSlider));

    addAndMakeVisible(generateIRButton);
    generateIRButton.onClick = [this] { raytracer.launchThread(Thread::Priority::high); };

    addAndMakeVisible(generateLSButton);
    generateLSButton.onClick = [this] { audioProcessor.ir.makeCopyOf(audioProcessor.generateLogarithmicSweep(20.0f, 20000.0f, 1.0f, audioProcessor.globalSampleRate, 2));
                                               impulseResponseComponent.updateThumbnail(audioProcessor.globalSampleRate); };

    settingsWindow.centreAroundComponent(this, settingsWindow.getWidth(), settingsWindow.getHeight());
    settingsWindow.setBackgroundColour(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));
    settingsWindow.addChangeListener(&impulseResponseComponent);

    auto normalSettings = Drawable::createFromImageData(BinaryData::settings_FILL0_wght100_GRAD25_opsz48_svg, BinaryData::settings_FILL0_wght100_GRAD25_opsz48_svgSize);
    auto overSettings   = Drawable::createFromImageData(BinaryData::settings_FILL0_wght100_GRAD200_opsz48_svg, BinaryData::settings_FILL0_wght100_GRAD200_opsz48_svgSize);
    auto downSettings   = Drawable::createFromImageData(BinaryData::settings_FILL0_wght100_GRAD200_opsz48_svg, BinaryData::settings_FILL0_wght100_GRAD200_opsz48_svgSize);

    normalSettings->replaceColour(Colour(0xff000000), getLookAndFeel().findColour(TextButton::textColourOffId));
    overSettings->replaceColour(Colour(0xff000000), getLookAndFeel().findColour(TextButton::textColourOffId));
    downSettings->replaceColour(Colour(0xff000000), getLookAndFeel().findColour(TextButton::textColourOnId));

    addAndMakeVisible(settingsButton);
    settingsButton.setImages(normalSettings.get(),
                             overSettings.get(),
                             downSettings.get());
    settingsButton.onClick = [this] { settingsWindow.setVisible(true); };

    objectWindow.centreAroundComponent(this, objectWindow.getWidth(), objectWindow.getHeight());
    objectWindow.setBackgroundColour(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    auto normalAdd = Drawable::createFromImageData(BinaryData::add_circle_FILL0_wght100_GRAD25_opsz48_svg, BinaryData::add_circle_FILL0_wght100_GRAD25_opsz48_svgSize);
    auto overAdd   = Drawable::createFromImageData(BinaryData::add_circle_FILL0_wght100_GRAD200_opsz48_svg, BinaryData::add_circle_FILL0_wght100_GRAD200_opsz48_svgSize);
    auto downAdd   = Drawable::createFromImageData(BinaryData::add_circle_FILL0_wght100_GRAD200_opsz48_svg, BinaryData::add_circle_FILL0_wght100_GRAD200_opsz48_svgSize);

    normalAdd->replaceColour(Colour(0xff000000), getLookAndFeel().findColour(TextButton::textColourOffId));
    overAdd->replaceColour(Colour(0xff000000), getLookAndFeel().findColour(TextButton::textColourOffId));
    downAdd->replaceColour(Colour(0xff000000), getLookAndFeel().findColour(TextButton::textColourOnId));

    addAndMakeVisible(addObjectButton);
    addObjectButton.setImages(normalAdd.get(),
                              overAdd.get(),
                              downAdd.get());
    addObjectButton.onClick = [this] { objectWindow.openWindow(ObjectWindow::Mode::ADD); };

    auto normalEdit = Drawable::createFromImageData(BinaryData::edit_FILL0_wght100_GRAD25_opsz48_svg, BinaryData::edit_FILL0_wght100_GRAD25_opsz48_svgSize);
    auto overEdit   = Drawable::createFromImageData(BinaryData::edit_FILL0_wght100_GRAD200_opsz48_svg, BinaryData::edit_FILL0_wght100_GRAD200_opsz48_svgSize);
    auto downEdit   = Drawable::createFromImageData(BinaryData::edit_FILL0_wght100_GRAD200_opsz48_svg, BinaryData::edit_FILL0_wght100_GRAD200_opsz48_svgSize);

    normalEdit->replaceColour(Colour(0xff000000), getLookAndFeel().findColour(TextButton::textColourOffId));
    overEdit->replaceColour(Colour(0xff000000), getLookAndFeel().findColour(TextButton::textColourOffId));
    downEdit->replaceColour(Colour(0xff000000), getLookAndFeel().findColour(TextButton::textColourOnId));

    addAndMakeVisible(editObjectButton);
    editObjectButton.setImages(normalEdit.get(),
                               overEdit.get(),
                               downEdit.get());
    editObjectButton.onClick = [this] { objectWindow.openWindow(ObjectWindow::Mode::EDIT); };

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
    auto area = getLocalBounds();

    {   // Visualizer
        auto visualizerArea = area.removeFromTop(4*area.getHeight()/5);
        openGLComponent.            setBounds(visualizerArea);
    }

    {   // Buttons
        auto buttonsArea = area.removeFromTop(1*area.getHeight()/3);
        settingsButton.             setBounds(buttonsArea.removeFromRight(buttonsArea.getHeight()).reduced(5));
        editObjectButton.           setBounds(buttonsArea.removeFromRight(buttonsArea.getHeight()).reduced(5));
        addObjectButton.            setBounds(buttonsArea.removeFromRight(buttonsArea.getHeight()).reduced(5));

        generateIRButton.           setBounds(buttonsArea.reduced(5));
    }

    {   // Waveform
        auto waveformArea = area;
        gainSlider.setBounds(waveformArea.removeFromRight(waveformArea.getHeight()));
        impulseResponseComponent.   setBounds(waveformArea);
    }
}
