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

    addAndMakeVisible(objectMenu);
    objectMenu.onChange = [this] { populateObjectProperties(); };

    addAndMakeVisible(addObjectButton);
    addObjectButton.setColour(TextButton::buttonColourId, Colour (0xff797fed));
    addObjectButton.setColour(TextButton::textColourOffId, Colours::black);
    addObjectButton.onClick = [this] { addObject(); };

    addAndMakeVisible(removeObjectButton);
    removeObjectButton.setColour(TextButton::buttonColourId, Colour (0xff797fed));
    removeObjectButton.setColour(TextButton::textColourOffId, Colours::black);
    removeObjectButton.onClick = [this] { removeObject(); };

    addAndMakeVisible(activeToggle);
    activeToggle.onStateChange = [this] { updateObjectProperties(); };
    activeToggleLabel.attachToComponent(&activeToggle, true);

    addAndMakeVisible(typeMenu);
    typeMenu.addItem("Microphone", Raytracer::Object::Type::MICROPHONE);
    typeMenu.addItem("Speaker", Raytracer::Object::Type::SPEAKER);

    addAndMakeVisible(xPositionSlider);
    xPositionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, xPositionSlider.getTextBoxWidth(), xPositionSlider.getTextBoxHeight());
    xPositionSlider.setSliderStyle(juce::Slider::LinearBar);
    xPositionSlider.setRange(-50.0f, 50.0f);
    xPositionSlider.setNumDecimalPlacesToDisplay(2);
    xPositionSlider.setMouseDragSensitivity(1000);
    xPositionSlider.onValueChange = [this] { updateObjectProperties(); };
    xPositionLabel.attachToComponent(&xPositionSlider, false);

    addAndMakeVisible(yPositionSlider);
    yPositionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, yPositionSlider.getTextBoxWidth(), yPositionSlider.getTextBoxHeight());
    yPositionSlider.setSliderStyle(juce::Slider::LinearBar);
    yPositionSlider.setRange(-50.0f, 50.0f);
    yPositionSlider.setNumDecimalPlacesToDisplay(2);
    yPositionLabel.attachToComponent(&yPositionSlider, false);

    addAndMakeVisible(zPositionSlider);
    zPositionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, zPositionSlider.getTextBoxWidth(), zPositionSlider.getTextBoxHeight());
    zPositionSlider.setSliderStyle(juce::Slider::LinearBar);
    zPositionSlider.setRange(-50.0f, 50.0f);
    zPositionSlider.setNumDecimalPlacesToDisplay(2);
    zPositionLabel.attachToComponent(&zPositionSlider, false);

    addAndMakeVisible(xRotationSlider);
    xRotationSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, xRotationSlider.getTextBoxWidth(), xRotationSlider.getTextBoxHeight());
    xRotationSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    xRotationSlider.setRange(-180.0f, 180.0f);
    xRotationSlider.setNumDecimalPlacesToDisplay(2);
    xRotationLabel.attachToComponent(&xRotationSlider, false);

    addAndMakeVisible(yRotationSlider);
    yRotationSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, yRotationSlider.getTextBoxWidth(), yRotationSlider.getTextBoxHeight());
    yRotationSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    yRotationSlider.setRange(-180.0f, 180.0f);
    yRotationSlider.setNumDecimalPlacesToDisplay(2);
    yRotationLabel.attachToComponent(&yRotationSlider, false);

    addAndMakeVisible(zRotationSlider);
    zRotationSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, zRotationSlider.getTextBoxWidth(), zRotationSlider.getTextBoxHeight());
    zRotationSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    zRotationSlider.setRange(-180.0f, 180.0f);
    zRotationSlider.setNumDecimalPlacesToDisplay(2);
    zRotationLabel.attachToComponent(&zRotationSlider, false);

    populateObjectMenu();
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

    objectMenu.                 setBounds(2*width/4, 0*height/12, 2*width/4, 1*height/12);
    addObjectButton.            setBounds(2*width/4, 1*height/12, 1*width/4, 1*height/12);
    removeObjectButton.         setBounds(3*width/4, 1*height/12, 1*width/4, 1*height/12);

    activeToggle.               setBounds(4*width/6, 2*height/12, 1*width/6, 1*height/12);
    typeMenu.                   setBounds(5*width/6, 2*height/12, 1*width/6, 1*height/12);

    xPositionSlider.            setBounds(3*width/6, 4*height/12, 1*width/6, 1*height/12);
    yPositionSlider.            setBounds(4*width/6, 4*height/12, 1*width/6, 1*height/12);
    zPositionSlider.            setBounds(5*width/6, 4*height/12, 1*width/6, 1*height/12);

    xRotationSlider.            setBounds(3*width/6, 6*height/12, 1*width/6, 1*height/12);
    yRotationSlider.            setBounds(4*width/6, 6*height/12, 1*width/6, 1*height/12);
    zRotationSlider.            setBounds(5*width/6, 6*height/12, 1*width/6, 1*height/12);

    gainSlider.setBounds(width - 75, height - 75, 75, 75);
}

void RaumsimulationAudioProcessorEditor::addObject()
{

}

void RaumsimulationAudioProcessorEditor::removeObject()
{
    for (int objectNum = 0; objectNum < raytracer.objects.size(); objectNum++) {
        if (raytracer.objects[objectNum].name == objectMenu.getText()) {
            raytracer.objects.erase(raytracer.objects.begin() + objectNum);
        }
    }

    populateObjectMenu();
}

void RaumsimulationAudioProcessorEditor::populateObjectMenu()
{
    objectMenu.clear();

    for (auto& object : raytracer.objects) {
        objectMenu.addItem(object.name, objectMenu.getNumItems() + 1);
    }
}

void RaumsimulationAudioProcessorEditor::populateObjectProperties()
{
    if (objectMenu.getSelectedId() != 0) {
        for (const auto& object : raytracer.objects) {
            if (object.name == objectMenu.getText()) {
                activeToggle.setToggleState(object.active, NotificationType::dontSendNotification);
                typeMenu.setSelectedId(object.type);

                xPositionSlider.setValue(object.position.x);
                yPositionSlider.setValue(object.position.y);
                zPositionSlider.setValue(object.position.z);
            }
        }
    }
}

void RaumsimulationAudioProcessorEditor::updateObjectProperties()
{
    if (objectMenu.getSelectedId() != 0) {
        for (auto& object : raytracer.objects) {
            if (object.name == objectMenu.getText()) {
                object.active = activeToggle.getToggleState();
                object.type = static_cast<Raytracer::Object::Type>(typeMenu.getSelectedId());

                object.position.x = (float) xPositionSlider.getValue();
                object.position.y = (float) yPositionSlider.getValue();
                object.position.z = (float) zPositionSlider.getValue();
            }
        }
    }
}
