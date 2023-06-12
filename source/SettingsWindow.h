#pragma once

#include "JuceHeader.h"
#include "PluginProcessor.h"

class SettingsWindow : public juce::DocumentWindow,
                       public ChangeBroadcaster
{
public:
    SettingsWindow(RaumsimulationAudioProcessor&, juce::AudioProcessorValueTreeState&, const String& name, Colour backgroundColour, int requiredButtons, bool addToDesktop);
    ~SettingsWindow();

private:
    class SettingsComponent : public juce::Component
    {
    public:
        explicit SettingsComponent(SettingsWindow& pw)
        : parentWindow(pw)
        {
            setSize(400, 300);

            addAndMakeVisible(generalSettingsLabel);
            generalSettingsLabel.setFont(juce::Font(16.0f, juce::Font::bold));

            addAndMakeVisible(samplerateLabel);
            addAndMakeVisible(samplerateValueLabel);
            samplerateValueLabel.setText(String(parentWindow.audioProcessor.globalSampleRate) + "Hz", dontSendNotification);

            addAndMakeVisible(cubeSizeLabel);
            addAndMakeVisible(cubeSizeSlider);
            cubeSizeSlider.setSliderStyle(juce::Slider::LinearBar);
            cubeSizeSlider.setRange(0.0f, 3.0f, 1.0f);
            cubeSizeSlider.setTooltip("Level of quality used for room size estimation.");
            cubeSizeSlider.onValueChange = [this] { parentWindow.parameters.state.setProperty("cube_size", cubeSizeSlider.getValue(), nullptr); };
            double cubeSize = parentWindow.parameters.state.getProperty("cube_size");
            cubeSizeSlider.setValue(cubeSize, dontSendNotification);


            addAndMakeVisible(raytracerSettingsLabel);
            raytracerSettingsLabel.setFont(juce::Font(16.0f, juce::Font::bold));

            addAndMakeVisible(raysPerSourceLabel);
            addAndMakeVisible(raysPerSourceSlider);
            raysPerSourceSlider.setSliderStyle(juce::Slider::LinearBar);
            raysPerSourceSlider.setRange(100.0f, 10000.0f, 1.0f);
            raysPerSourceSlider.setTooltip("Number of rays that are cast for every active sound source.");
            raysPerSourceSlider.onValueChange = [this] { parentWindow.parameters.state.setProperty("rays_per_source", raysPerSourceSlider.getValue(), nullptr); };
            double raysPerSource = parentWindow.parameters.state.getProperty("rays_per_source");
            raysPerSourceSlider.setValue(raysPerSource, dontSendNotification);

            addAndMakeVisible(pointsInVisualizerLabel);
            addAndMakeVisible(pointsInVisualizerSlider);
            pointsInVisualizerSlider.setSliderStyle(juce::Slider::LinearBar);
            pointsInVisualizerSlider.setTextValueSuffix("%");
            pointsInVisualizerSlider.setRange(0.0f, 100.0f, 10.0f);
            pointsInVisualizerSlider.setTooltip("Percentage of reflection points that are shown in the visualizer.");
            pointsInVisualizerSlider.onValueChange = [this] { parentWindow.parameters.state.setProperty("points_in_visualizer", pointsInVisualizerSlider.getValue(), nullptr); };
            double pointsInVisualizer = parentWindow.parameters.state.getProperty("points_in_visualizer");
            pointsInVisualizerSlider.setValue(pointsInVisualizer, dontSendNotification);


            addAndMakeVisible(irSettingsLabel);
            irSettingsLabel.setFont(juce::Font(16.0f, juce::Font::bold));

            addAndMakeVisible(linesInWaveformLabel);
            addAndMakeVisible(linesInWaveformSlider);
            linesInWaveformSlider.setSliderStyle(juce::Slider::LinearBar);
            linesInWaveformSlider.setRange(0.0f, 15.0f, 1.0f);
            linesInWaveformSlider.setTooltip("Number of scale lines that are displayed with the waveform.");
            linesInWaveformSlider.onValueChange = [this] { parentWindow.parameters.state.setProperty("lines_in_waveform", linesInWaveformSlider.getValue(), nullptr);
                                                           parentWindow.sendChangeMessage(); };
            double linesInWaveform = parentWindow.parameters.state.getProperty("lines_in_waveform");
            linesInWaveformSlider.setValue(linesInWaveform, dontSendNotification);

            addAndMakeVisible(stereoLabel);
            addAndMakeVisible(stereoToggle);
            stereoToggle.setTooltip("Whether to generate a stereo impulse response.");
            stereoToggle.onStateChange = [this] { parentWindow.parameters.state.setProperty("stereo_ir", stereoToggle.getToggleState(), nullptr);  };
            bool stereoIR = parentWindow.parameters.state.getProperty("stereo_ir");
            stereoToggle.setToggleState(stereoIR, dontSendNotification);

            addAndMakeVisible(whiteNoiseLabel);
            addAndMakeVisible(whiteNoiseToggle);
            whiteNoiseToggle.setTooltip("Whether to use white noise or dirac impulses for generating impulse responses.");
            whiteNoiseToggle.onStateChange = [this] { parentWindow.parameters.state.setProperty("use_white_noise", whiteNoiseToggle.getToggleState(), nullptr);  };
            bool useWhiteNoise = parentWindow.parameters.state.getProperty("use_white_noise");
            whiteNoiseToggle.setToggleState(useWhiteNoise, dontSendNotification);
        };

        void paint(juce::Graphics & g) override
        {
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced(5);
            float labelWidthRatio = 0.5f;

            {   // General Settings
                auto generalSettingsArea = area.removeFromTop(75);
                generalSettingsLabel.           setBounds(generalSettingsArea.removeFromTop(25));

                auto samplerateArea = generalSettingsArea.removeFromTop(25);
                samplerateLabel.                setBounds(samplerateArea.removeFromLeft((int) (labelWidthRatio * (float) samplerateArea.getWidth())));
                samplerateValueLabel.           setBounds(samplerateArea);

                auto cubeSizeArea = generalSettingsArea.removeFromTop(25);
                cubeSizeLabel.                  setBounds(cubeSizeArea.removeFromLeft((int) (labelWidthRatio * (float) cubeSizeArea.getWidth())));
                cubeSizeSlider.                 setBounds(cubeSizeArea);
            }

            {   // Raytracer Settings
                auto raytracerSettingsArea = area.removeFromTop(75);
                raytracerSettingsLabel.         setBounds(raytracerSettingsArea.removeFromTop(25));

                auto raysPerSourceArea = raytracerSettingsArea.removeFromTop(25);
                raysPerSourceLabel.             setBounds(raysPerSourceArea.removeFromLeft((int) (labelWidthRatio * (float) raysPerSourceArea.getWidth())));
                raysPerSourceSlider.            setBounds(raysPerSourceArea);

                auto pointsInVisualizerArea = raytracerSettingsArea.removeFromTop(25);
                pointsInVisualizerLabel.        setBounds(pointsInVisualizerArea.removeFromLeft((int) (labelWidthRatio * (float) pointsInVisualizerArea.getWidth())));
                pointsInVisualizerSlider.       setBounds(pointsInVisualizerArea);
            }

            {   // IR Settings
                auto irSettingsArea = area.removeFromTop(100);
                irSettingsLabel.                setBounds(irSettingsArea.removeFromTop(25));

                auto linesInWaveformArea = irSettingsArea.removeFromTop(25);
                linesInWaveformLabel.           setBounds(linesInWaveformArea.removeFromLeft((int) (labelWidthRatio * (float) linesInWaveformArea.getWidth())));
                linesInWaveformSlider.          setBounds(linesInWaveformArea);

                auto stereoIRArea = irSettingsArea.removeFromTop(25);
                stereoLabel.                    setBounds(stereoIRArea.removeFromLeft((int) (labelWidthRatio * (float) stereoIRArea.getWidth())));
                stereoToggle.                   setBounds(stereoIRArea);

                auto whiteNoiseArea = irSettingsArea.removeFromTop(25);
                whiteNoiseLabel.                setBounds(whiteNoiseArea.removeFromLeft((int) (labelWidthRatio * (float) whiteNoiseArea.getWidth())));
                whiteNoiseToggle.               setBounds(whiteNoiseArea);
            }
        }

    private:
        SettingsWindow& parentWindow;

        Label           generalSettingsLabel{{}, "General"};
        Label           samplerateLabel{{}, "Rendering Samplerate"};
        Label           samplerateValueLabel{{}, "0"};
        Label           cubeSizeLabel{{}, "Volume Estimation Quality Level"};
        Slider          cubeSizeSlider;


        Label           raytracerSettingsLabel{{}, "Raytracer"};
        Label           raysPerSourceLabel{{}, "Rays per Source"};
        Slider          raysPerSourceSlider;
        Label           pointsInVisualizerLabel{{}, "Points in Visualizer"};
        Slider          pointsInVisualizerSlider;

        Label           irSettingsLabel{{}, "Impulse Response"};
        Label           linesInWaveformLabel{{}, "Lines in waveform display"};
        Slider          linesInWaveformSlider;
        Label           stereoLabel{{}, "Generate Stereo IRs"};
        ToggleButton    stereoToggle;
        Label           whiteNoiseLabel{{}, "Use white noise"};
        ToggleButton    whiteNoiseToggle;
    };

    void closeButtonPressed() override;

    RaumsimulationAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& parameters;
    SettingsComponent settingsComponent{*this};
};
