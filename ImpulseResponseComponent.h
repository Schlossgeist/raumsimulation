#pragma once

#include <JuceHeader.h>

class ImpulseResponseComponent : public juce::Component
{
public:
    ImpulseResponseComponent();

    void paint (juce::Graphics&) override;
    void resized() override;

private:

    TextButton wavFileLoadButton = { "Load WAV File..." , "Choose a file that contains the impulse response you want to apply"};
    std::unique_ptr<juce::FileChooser> wavFileChooser;
};
