# pragma once

#include <JuceHeader.h>
#include "externals/glm/glm/glm.hpp"
#include "externals/glm/glm/ext.hpp"
#include "WavefrontObjParser.h"
#include "PluginProcessor.h"
#include "CustomDatatypes.h"

class Raytracer : public juce::ThreadWithProgressWindow
{
public:
    Raytracer(RaumsimulationAudioProcessor&, juce::AudioProcessorValueTreeState&, const String &windowTitle, bool hasProgressBar, bool hasCancelButton, int timeOutMsWhenCancelling, const String &cancelButtonText, Component *componentToCentreAround);

    struct Hit {
        bool hitSurface = false;
        float distance = 1000000.0f;
        glm::vec3 hitPoint;
        glm::vec3 normal;
        MaterialProperties materialProperties;
    };

    struct SecondarySource {
        int order = 0;
        glm::vec3 position;
        glm::vec3 normal;
        float scatterCoefficient;
        Band6Coefficients energy_coefficients;
        float delayMS = 0.0f;
    };

    struct EnergyPortion {
        Band6Coefficients energy_coefficients;
        float delayMS = 0.0f;

        static bool byTotalEnergy (EnergyPortion a, EnergyPortion b)
        {
            float totalA = 0;
            for (float coefficient : a.energy_coefficients.coefficients) {
                totalA += coefficient;
            }

            float totalB = 0;
            for (float coefficient : a.energy_coefficients.coefficients) {
                totalB += coefficient;
            }

            return (totalA < totalB);
        }

        static bool byDelay (EnergyPortion a, EnergyPortion b)
        {
            return (a.delayMS < b.delayMS);
        }
    };

    struct Directivity {
        enum Pattern {
            OMNIDIRECTIONAL
        };

        Pattern pattern = OMNIDIRECTIONAL;
        glm::vec3 orientation;
    };

    struct Object {
        enum Type {
            MICROPHONE = 1,
            SPEAKER = 2
        };

        String name;
        Type type;
        bool active = false;
        glm::vec3 position = {0.0f, 0.0f, 0.0f};
    };

    const int maxBounces = 10;
    const int raysPerSource = 1000;
    const float speedOfSoundMpS = 343.0f;

    struct Ray {
        glm::vec3 position;
        glm::vec3 direction;
    };

    struct Triangle {
        glm::vec3 pointA, pointB, pointC;
        glm::vec3 normal;
    };

    void run() override;
    void setRoom(const File& objFile);

    std::vector<Object> objects;
    std::map<String, std::vector<EnergyPortion>> histograms;
    std::vector<SecondarySource> secondarySources;

private:

    RaumsimulationAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& parameters;

    WavefrontObjFile room;

    juce::Random randomGenerator;
    float randomNormalDistribution();

    void trace(Ray ray);
    Hit calculateBounce(Ray ray);
    bool checkVisibility(glm::vec3 positionA, glm::vec3 positionB);

    static Hit collisionTriangle(Ray ray, Triangle triangle);
};
