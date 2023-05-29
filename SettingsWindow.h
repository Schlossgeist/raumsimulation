#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SettingsWindow : public juce::DocumentWindow
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


            addAndMakeVisible(raytracerSettingsLabel);
            raytracerSettingsLabel.setFont(juce::Font(16.0f, juce::Font::bold));

            addAndMakeVisible(raysPerSourceLabel);
            addAndMakeVisible(raysPerSourceSlider);
            raysPerSourceSlider.setSliderStyle(juce::Slider::LinearBar);
            raysPerSourceSlider.setRange(100.0f, 10000.f, 1.0f);
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
        };

        void paint(juce::Graphics & g) override
        {
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced(5);

            auto generalSettingsArea = area.removeFromTop(100);
            generalSettingsLabel.           setBounds(generalSettingsArea.removeFromTop(25));

                auto samplerateArea = generalSettingsArea.removeFromTop(25);
                samplerateLabel.            setBounds(samplerateArea.removeFromLeft(samplerateArea.getWidth()/3));
                samplerateValueLabel.       setBounds(samplerateArea);

            auto raytracerSettingsArea = area.removeFromTop(200);
            raytracerSettingsLabel.         setBounds(raytracerSettingsArea.removeFromTop(25));

                auto raysPerSourceArea = raytracerSettingsArea.removeFromTop(25);
                raysPerSourceLabel.         setBounds(raysPerSourceArea.removeFromLeft(raysPerSourceArea.getWidth()/3));
                raysPerSourceSlider.        setBounds(raysPerSourceArea);

                auto pointsInVisualizerArea = raytracerSettingsArea.removeFromTop(25);
                pointsInVisualizerLabel.    setBounds(pointsInVisualizerArea.removeFromLeft(pointsInVisualizerArea.getWidth()/3));
                pointsInVisualizerSlider.   setBounds(pointsInVisualizerArea);
        }

    private:
        SettingsWindow& parentWindow;

        Label       generalSettingsLabel{{}, "General"};
        Label       samplerateLabel{{}, "Rendering Samplerate"};
        Label       samplerateValueLabel{{}, "0"};

        Label       raytracerSettingsLabel{{}, "Raytracer"};
        Label       raysPerSourceLabel{{}, "Rays per Source"};
        Slider      raysPerSourceSlider;
        Label       pointsInVisualizerLabel{{}, "Points in Visualizer"};
        Slider      pointsInVisualizerSlider;
    };

    void closeButtonPressed() override;

    RaumsimulationAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& parameters;
    SettingsComponent settingsComponent{*this};
};
