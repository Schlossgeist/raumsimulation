#include "ObjectWindow.h"

ObjectWindow::ObjectWindow(RaumsimulationAudioProcessor& p, juce::AudioProcessorValueTreeState& pts, Raytracer& rt, const String& name, Colour backgroundColour, int requiredButtons, bool addToDesktop)
    : audioProcessor(p)
    , parameters(pts)
    , raytracer(rt)
    , DocumentWindow(name, backgroundColour, requiredButtons, addToDesktop)
{
    setUsingNativeTitleBar(true);
    setIcon(ImageFileFormat::loadFrom(BinaryData::icon_png, BinaryData::icon_pngSize));
    setContentOwned(&objectComponent, true);
}

ObjectWindow::~ObjectWindow()
{
}

void ObjectWindow::openWindow(ObjectWindow::Mode mode)
{
    setVisible(true);
    currentMode = mode;

    if (mode == ADD) {
        juce::DocumentWindow::setName("Add");
        objectComponent.clearObjectProperties();
    }

    if (mode == EDIT) {
        juce::DocumentWindow::setName("Edit");
        objectComponent.populateObjectProperties();
    }

    objectComponent.populateObjectMenu();
}

void ObjectWindow::closeButtonPressed()
{
    setVisible(false);
}
