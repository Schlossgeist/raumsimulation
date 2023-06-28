#include "SettingsWindow.h"

SettingsWindow::SettingsWindow(RaumsimulationAudioProcessor& p, juce::AudioProcessorValueTreeState& pts, const String& name, Colour backgroundColour, int requiredButtons, bool addToDesktop)
    : audioProcessor(p)
    , parameters(pts)
    , DocumentWindow(name, backgroundColour, requiredButtons, addToDesktop)
{
    setUsingNativeTitleBar(true);
    setIcon(ImageFileFormat::loadFrom(BinaryData::icon_png, BinaryData::icon_pngSize));
    setContentOwned(&settingsComponent, true);
}

SettingsWindow::~SettingsWindow()
{
}

void SettingsWindow::closeButtonPressed()
{
    setVisible(false);
}
