/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class RaumsimulationAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    RaumsimulationAudioProcessor();
    ~RaumsimulationAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RaumsimulationAudioProcessor)

    juce::Random randomGenerator;

    juce::ValueTree settings
            { "SettingsGroups", {},
             {
                     { "SettingsGroup", {{ "name", "Graphics Settings" }},
                      {
                              { "Setting", {{ "id", "zoom" },               { "value", 0.5 }}},
                              { "Setting", {{ "id", "rotation_speed" },     { "value", 0.5 }}},
                              { "Setting", {{ "id", "obj_file_url" },     { "value", "" }}}
                      }
                     }
             }
            };
    juce::AudioProcessorValueTreeState parameters;

    std::atomic<float>* gainParameter = nullptr;
};
