# pragma once

#include "CustomDatatypes.h"
#include "ImpulseResponseComponent.h"
#include "PluginProcessor.h"
#include "WavefrontObjParser.h"
#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <JuceHeader.h>

class Raytracer : public juce::ThreadWithProgressWindow,
                  public ChangeBroadcaster
{
public:
    Raytracer(RaumsimulationAudioProcessor&, juce::AudioProcessorValueTreeState&, ImpulseResponseComponent&, const String &windowTitle, bool hasProgressBar, bool hasCancelButton, int timeOutMsWhenCancelling, const String &cancelButtonText, Component *componentToCentreAround);

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
        Band6Coefficients energyCoefficients;
        float delayMS = 0.0f;
    };

    struct EnergyPortion {
        Band6Coefficients energyCoefficients;
        float delayMS = 0.0f;

        static bool byTotalEnergy (EnergyPortion a, EnergyPortion b)
        {
            float totalA = 0;
            for (float coefficient : a.energyCoefficients.coefficients) {
                totalA += coefficient;
            }

            float totalB = 0;
            for (float coefficient : b.energyCoefficients.coefficients) {
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

    int raysPerSource = 1000;
    const float speedOfSoundMpS = 343.0f;
    int minOrder = 1;
    int maxOrder = 1;

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
    void clear();
    void saveObjects();
    void restoreObjects();

    std::vector<Object> objects;
    std::map<String, std::vector<EnergyPortion>> histograms;

    std::vector<EnergyPortion> extractHistogramSlice(double startTimeMS, double endTimeMS, const std::vector<EnergyPortion>& energyPortions)
    {
        // energyPortions has to be sorted byDelay

        std::vector<EnergyPortion> result;

        for (auto& energyPortion : energyPortions) {
            if (endTimeMS < energyPortion.delayMS) {
                break;
            }

            if (startTimeMS < energyPortion.delayMS && energyPortion.delayMS < endTimeMS) {
                result.push_back(energyPortion);
            }
        }

        return result;
    }

    std::vector<SecondarySource> secondarySources;

    struct Hash {
        static unsigned cantor(unsigned int a, unsigned int b) {
            return (a + b) / 2 * (a + b +1) + b;
        }

        //  X:  0 | -1 |  1 | -2 |  2 | -3 |  3 | -4 |  4 | -5 |  5 | ...
        //  Y:  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | ...
        static unsigned int mirror(int x) {
            if (x >= 0) {
                return 2*x;
            } else {
                return 2*x-1;
            }
        }

        unsigned long long operator()(const glm::ivec3 &vector) const {
            return cantor(mirror(vector.x), cantor(mirror(vector.y), mirror(vector.z)));
        }
    };

    std::unordered_set<glm::ivec3, Hash> cubes;
    int cubeSizeCM = 100;

private:

    RaumsimulationAudioProcessor& audioProcessor;
    ImpulseResponseComponent& impulseResponseComponent;
    juce::AudioProcessorValueTreeState& parameters;

    WavefrontObjFile room;

    juce::Random randomGenerator;
    float randomNormalDistribution();


    float flood(glm::ivec3 startPoint);
    std::vector<glm::ivec3> findNeighbors(glm::ivec3 cube);

    void trace(Ray ray);
    Hit calculateBounce(Ray ray);
    bool checkVisibility(glm::vec3 positionA, glm::vec3 positionB);

    static Hit collisionTriangle(Ray ray, Triangle triangle);
};
