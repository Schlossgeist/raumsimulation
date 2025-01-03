#pragma once

#include "JuceHeader.h"
#include "PluginProcessor.h"

class ImpulseResponseComponent : public juce::Component,
                                 public ChangeListener,
                                 public ChangeBroadcaster,
                                 public SettableTooltipClient
{
public:
    ImpulseResponseComponent(RaumsimulationAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~ImpulseResponseComponent();

    void paint(juce::Graphics&) override;
    void resized() override;
    void updateThumbnail(double sampleRate);

private:
    void openFile();
    void saveFile();
    void setURL(const URL&);
    void changeListenerCallback(ChangeBroadcaster*) override;
    void mouseMove(const MouseEvent& e) override;

    RaumsimulationAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& parameters;

    AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache{3};
    juce::AudioThumbnail thumbnail;

    juce::URL irFileURL = {};

    TextButton playPauseButton = {"Play", "Play the currently loaded impulse response"};
    Label irFileLabel = {{}, "No file selected"};
    TextButton irFileLoadButton = {"Load audio File..." , "Choose a file that contains the impulse response you want to apply"};
    TextButton irFileSaveButton = {"Save impulse response..." , "Save the currently loaded impulse response on your computer"};
    std::unique_ptr<juce::FileChooser> irFileChooser;
    TextButton clearButton = {"Clear", "Clear the impulse response buffer"};

    Label irSizeLabel;
};
