#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class ImpulseResponseComponent : public juce::Component,
                                 public ChangeListener,
                                 public ChangeBroadcaster
{
public:
    ImpulseResponseComponent(RaumsimulationAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~ImpulseResponseComponent();

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void openFile();
    void setURL(const URL&);
    void changeListenerCallback(ChangeBroadcaster*) override;

    RaumsimulationAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& parameters;

    AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache{3};
    juce::AudioThumbnail thumbnail;

    juce::URL irFileURL = {};

    Label irFileLabel = {{}, "No file selected"};
    TextButton irFileLoadButton = {"Load audio File..." , "Choose a file that contains the impulse response you want to apply"};
    std::unique_ptr<juce::FileChooser> irFileChooser;

    Label irSizeLabel;
};
