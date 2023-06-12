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

    void updateParameters();
    void reset() override;
    void playIR();
    void clearIR();
    juce::AudioBuffer<float>& generateLogarithmicSweep(double startFrequency, double endFrequency, float lengthS, double sampleRate, int numChannels);

    juce::AudioBuffer<float> ir;
    int irBufferPosition = 0;
    bool play = false;

    double globalSampleRate = 0.0f;

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
                     },
                     { "SettingsGroup", {{ "name", "General Settings" }},
                      {
                              { "Setting", {{ "id", "cube_size" },     { "value", 4.0f }}}
                      }
                     },
                     { "SettingsGroup", {{ "name", "Room Settings" }},
                      {
                              { "Setting", {{ "id", "ir_file_url" },     { "value", "" }}}
                      }
                     },
                     { "SettingsGroup", {{ "name", "Raytracer Settings" }},
                      {
                              { "Setting", {{ "id", "rays_per_source" },     { "value", 1000.0 }}},
                              { "Setting", {{ "id", "points_in_visualizer" },     { "value", 50.0 }}}
                      }
                     },
                     { "SettingsGroup", {{ "name", "IR Settings" }},
                      {
                              { "Setting", {{ "id", "lines_in_waveform" },     { "value", 10.0 }}},
                              { "Setting", {{ "id", "stereo_ir" },     { "value", false }}},
                              { "Setting", {{ "id", "use_white_noise" },     { "value", true }}}
                      }
                     }
             }
            };
    juce::AudioProcessorValueTreeState parameters;

    std::atomic<float>* gainParameter = nullptr;


    juce::dsp::Convolution convolution{dsp::Convolution::NonUniform{512}};
    juce::dsp::Gain<float> gain;

    std::atomic<int> irSize{0};
};
