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

    struct Directivity {
        enum Pattern {
            OMNIDIRECTIONAL
        };

        Pattern pattern = OMNIDIRECTIONAL;
        glm::vec3 orientation;
    };

    struct Microphone {
        bool active = false;
        glm::vec3 position = {0.0, 0.0, 0.0};
        Directivity directivity;
    };

    struct Speaker {
        bool active = false;
        glm::vec3 position = {0.0, 0.0, 0.0};
        Directivity directivity;
        double power = 10.0;
    };

    const int maxBounces = 10;
    const int raysPerMicrophone = 100000;
    const float speedOfSoundMpS = 343.0f;

    struct Ray {
        glm::vec3 position;
        glm::vec3 direction;
    };

    struct Hit {
        enum SurfaceType {
            NONE = 0,
            WALL = 1,
            SPEAKER = 2
        };

        SurfaceType surface = NONE;
        float distance = 1000000.0f;
        glm::vec3 hitPoint;
        glm::vec3 normal;
        Band6Coefficients absorption_coefficients;
    };

    struct Reflection {
        bool didHit = false;
        Band6Coefficients energy_coefficients;
        float delayMS = 0.0f;
    };

    struct Triangle {
        glm::vec3 pointA, pointB, pointC;
        glm::vec3 normal;
    };

    void run() override;
    void setRoom(const File& objFile);

    Array<Microphone> microphones;
    Array<Speaker> speakers;

private:

    RaumsimulationAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& parameters;

    WavefrontObjFile room;

    juce::Random randomGenerator;
    float randomNormalDistribution();

    std::vector<Reflection> reflections;

    Reflection trace(Ray ray);
    Hit calculateBounce(Ray ray);

    static Hit collisionSphere(Ray ray, glm::vec3 position, float radius);
    static Hit collisionTriangle(Ray ray, Triangle triangle);
};
