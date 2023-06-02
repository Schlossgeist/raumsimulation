#include "Raytracer.h"
#include "ImpulseResponseComponent.h"
#include "externals/glm/glm/gtx/vector_angle.hpp"

Raytracer::Raytracer(RaumsimulationAudioProcessor& p, juce::AudioProcessorValueTreeState& pts, ImpulseResponseComponent& irc, const String &windowTitle, bool hasProgressBar, bool hasCancelButton, int timeOutMsWhenCancelling, const String &cancelButtonText, Component *componentToCentreAround)
    : audioProcessor(p)
    , parameters(pts)
    , impulseResponseComponent(irc)
    , ThreadWithProgressWindow(windowTitle, hasProgressBar, hasCancelButton, timeOutMsWhenCancelling, cancelButtonText, componentToCentreAround)
{
    objects.push_back({"Mic1", Object::Type::MICROPHONE, true, glm::vec3{2.5f, 3.5f, 2.0f}});
    objects.push_back({"Spk1", Object::Type::SPEAKER, true, glm::vec3{7.0f, -1.0f, 3.0f}});

    objects.push_back({"Mic2", Object::Type::MICROPHONE, false, glm::vec3{0.5f, 0.5f, 2.0f}});
    objects.push_back({"Spk2", Object::Type::SPEAKER, false, glm::vec3{3.0f, 1.0f, 3.0f}});
}

void Raytracer::setRoom(const File& objFile)
{
    room.load(objFile);
}

void Raytracer::run()
{
    setStatusMessage("Loading room model...");
    auto const objFileURL = static_cast<const juce::URL>(parameters.state.getProperty("obj_file_url"));
    setRoom(objFileURL.getLocalFile());
    sleep(1000);

    String activeMicrophoneName;

    for (const auto& object : objects) {
        if (object.type == Object::MICROPHONE && object.active) {
            activeMicrophoneName = object.name;
            break;
        }
    }

    if (!activeMicrophoneName.isEmpty()) {
        setStatusMessage("Using " + activeMicrophoneName + " for IR generation");
        sleep(1000);
    } else {
        setStatusMessage("No active microphone found. Terminating...");
        sleep(1000);
        return;
    }

    //========================= RAY TRACING =========================//
    {
        std::vector<Object> speakers;

        for (const auto& object : objects) {
            if (object.type == Object::Type::SPEAKER && object.active) {
                speakers.push_back(object);
            }
        }

        setStatusMessage("Casting rays...");

        for (int speakerNum = 0; speakerNum < speakers.size(); speakerNum++) {
            setStatusMessage("Casting Rays for source " + String(speakerNum + 1) + " / " + String(speakers.size()));

            // add source for direct sound
            secondarySources.push_back({0, speakers[speakerNum].position, glm::vec3(), 0.0f, Band6Coefficients(), 0.0f});

            for (int rayNum = 0; rayNum < raysPerSource; rayNum++) {
                // generate Ray at speaker position with random direction
                Ray randomRay = {
                        speakers[speakerNum].position,
                        glm::normalize(glm::vec3{randomNormalDistribution(), randomNormalDistribution(), randomNormalDistribution()})
                };

                trace(randomRay);

                // update the progress bar on the dialog box
                setProgress((float) ((speakerNum + 1) * (rayNum + 1)) / (float) (speakers.size() * raysPerSource));
            }
        }
    }

    //========================= ROOM VOLUME ESTIMATION =========================//
    float roomVolumeM3 = 0.0f;
    {
        float minX = 0.0f;
        float minY = 0.0f;
        float minZ = 0.0f;
        float maxX = 0.0f;
        float maxY = 0.0f;
        float maxZ = 0.0f;

        for (auto secondarySource : secondarySources) {
            glm::vec3 sp = secondarySource.position;
            if (sp.x < minX) minX = sp.x;
            if (sp.y < minY) minY = sp.y;
            if (sp.z < minZ) minZ = sp.z;
            if (sp.x > maxX) maxX = sp.x;
            if (sp.y > maxY) maxY = sp.y;
            if (sp.z > maxZ) maxZ = sp.z;
        }

        roomVolumeM3 = (abs(minX) + abs(maxX)) * (abs(minY) + abs(maxY)) * (abs(minZ) + abs(maxZ));
        roomVolumeM3 = floor(roomVolumeM3 / 10.0f) * 10.0f;

        setStatusMessage("Estimated room size: " + String(roomVolumeM3) + " cubic meters");
        sleep(1000);
    }

    //========================= GATHERING =========================//
    {
        std::vector<Object> microphones;

        for (const auto& object : objects) {
            if (object.type == Object::Type::MICROPHONE && object.active) {
                microphones.push_back(object);
                histograms.insert(std::pair{object.name, std::vector<EnergyPortion>()});
            }
        }

        for (int microphoneNum = 0; microphoneNum < microphones.size(); microphoneNum++) {
            auto microphone = microphones[microphoneNum];

            // user pressed "cancel"
            if (threadShouldExit())
                break;

            setStatusMessage("Gathering energy contributions for receiver " + String(microphoneNum + 1) + " / " + String(microphones.size()));

            for (int secondarySourceNum = 0; secondarySourceNum < secondarySources.size(); secondarySourceNum++) {
                auto secondarySource = secondarySources[secondarySourceNum];

                // user pressed "cancel"
                if (threadShouldExit())
                    break;

                if (checkVisibility(secondarySource.position, microphone.position)) {
                    if (secondarySource.order > 0) {
                        // lamberts cosine law:
                        // energy received at the observers is proportional to the cosine of the angle between the reflection vector and the surface normal
                        glm::vec3 edgeSM = glm::normalize(microphone.position - secondarySource.position);
                        float     angle  = glm::angle(glm::normalize(secondarySource.normal), edgeSM);
                        secondarySource.energyCoefficients *= cos(angle);
                    }

                    secondarySource.delayMS += glm::length(secondarySource.position - microphone.position) / speedOfSoundMpS * 1000.0f;
                    histograms.at(microphone.name).push_back({secondarySource.energyCoefficients, secondarySource.delayMS});
                }

                // update the progress bar on the dialog box
                setProgress((float) ((microphoneNum + 1) * (secondarySourceNum + 1)) / (float) (microphones.size() * secondarySources.size()));
            }
        }
    }

    //========================= GENERATING =========================//
    AudioBuffer<float> buffer;
    {
        setStatusMessage("Generating impulse response...");

        std::sort(histograms.at(activeMicrophoneName).begin(), histograms.at(activeMicrophoneName).end(), EnergyPortion::byDelay);

        double latestReflectionS = histograms.at(activeMicrophoneName)[histograms.at(activeMicrophoneName).size() - 1].delayMS / 1000.0f;

        buffer.setSize(audioProcessor.ir.getNumChannels(),
                       (int) (audioProcessor.globalSampleRate * (latestReflectionS + 0.1f)),
                       false,
                       true,
                       false);

        double endOfPreviousIntervalMS = histograms.at(activeMicrophoneName)[0].delayMS;

        setStatusMessage("Generating dirac sequence...");

        while (endOfPreviousIntervalMS < latestReflectionS * 1000.0f) {
            // random value from nextDouble() is in range 0 (inclusive) to 1.0 (exclusive),
            // but here range 0 (exclusive) to 1.0 (inclusive) is needed because this value is used as a denominator
            double randomNumber = abs(randomGenerator.nextDouble() - 1);
            double currentTimeMS = endOfPreviousIntervalMS;

            /**
             * @see Section 5.3.4 in Dirk Schröder, Physically Based Real-Time Auralization of Interactive Virtual Environments
             */

            double speedOfSound3MpS = speedOfSoundMpS * speedOfSoundMpS * speedOfSoundMpS;
            double currentTime2S = (currentTimeMS / 1000.0f) * (currentTimeMS / 1000.0f);
            double volumeM3 = 650.0f;

            double u = (4.0f * glm::pi<double>() * speedOfSound3MpS * currentTime2S) / volumeM3;

            if (u > 10000.0f) {
                u = 10000.0f;
            }

            double intervalSizeMS = 1 / u * log(1 / randomNumber) * 1000.0f;

            double lengthOfSampleMS = 1 / audioProcessor.globalSampleRate * 1000.0f;

            if (intervalSizeMS < lengthOfSampleMS) {
                intervalSizeMS = lengthOfSampleMS;
            }

            float dirac = 1.0f;

            if (randomGenerator.nextDouble() > 0.5f) {
                dirac = -1.0f;
            }

            double eventTimeMS = currentTimeMS + randomGenerator.nextDouble() * intervalSizeMS;

            int sample = (int) (eventTimeMS * audioProcessor.globalSampleRate / 1000.0f);

            auto *writePtrArray = buffer.getArrayOfWritePointers();
            for (int channel = 0; channel < buffer.getNumChannels(); channel++) {
                writePtrArray[channel][sample] = dirac;
            }

            endOfPreviousIntervalMS += intervalSizeMS;
        }

        sleep(1000);
    }

    audioProcessor.ir = buffer;
    impulseResponseComponent.updateThumbnail(audioProcessor.globalSampleRate);

    {
        AudioBuffer<float> gainCurveBuffers[6] = {buffer, buffer, buffer, buffer, buffer, buffer};

        float gain = 0.0f;
        for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
            double startTimeMS = sample / audioProcessor.globalSampleRate * 1000.0f;
            double endTimeMS = startTimeMS + 1000.0f/audioProcessor.globalSampleRate;
            auto slice = extractHistogramSlice(startTimeMS, endTimeMS, activeMicrophoneName);

            for (int channel = 0; channel < buffer.getNumChannels(); channel++) {
                for (int i = 0; i < 6; i++) {
                    setStatusMessage("Calculating gain curve for sample " + String(sample+1) + "/" + String(audioProcessor.ir.getNumSamples()));
                    auto *writePtrArray = gainCurveBuffers[i].getArrayOfWritePointers();

                    if (!slice.empty()) {
                        gain = 0.0f;
                        for (int ep = 0; ep < slice.size(); ep++) {
                            gain += slice[ep].energyCoefficients[i];
                        }
                        gain /= (float) slice.size();
                    }

                    writePtrArray[channel][sample] = gain;
                }
            }
        }

        AudioBuffer<float> bandBuffers[6] = {buffer, buffer, buffer, buffer, buffer, buffer};

        for (int i = 0; i < 6; i++) {
            double centerFrequency = pow(2, i)*125;
            setStatusMessage("Filtering forwards for band " + String(i+1) + "/6 (" + String(centerFrequency) + " Hz)");

            IIRFilter filter;
            filter.setCoefficients(juce::IIRCoefficients::makeBandPass(
                    audioProcessor.globalSampleRate,
                    centerFrequency,
                    1.0f/sqrt(2.0f)));

            auto *writePtrArray = bandBuffers[i].getArrayOfWritePointers();
            for (int channel = 0; channel < bandBuffers[i].getNumChannels(); channel++) {
                filter.processSamples(writePtrArray[channel], bandBuffers[i].getNumSamples());
            }
        }

        setStatusMessage("Reversing bands...");
        for (int i = 0; i < 6; i++) {
            bandBuffers[i].reverse(0, bandBuffers[i].getNumSamples() - 1);
        }

        for (int i = 0; i < 6; i++) {
            double centerFrequency = pow(2, i)*125;
            setStatusMessage("Filtering backwards for band " + String(i+1) + "/6 (" + String(centerFrequency) + " Hz)");

            IIRFilter filter;
            filter.setCoefficients(juce::IIRCoefficients::makeBandPass(
                    audioProcessor.globalSampleRate,
                    centerFrequency,
                    1.0f/sqrt(2.0f)));

            auto *writePtrArray = bandBuffers[i].getArrayOfWritePointers();
            for (int channel = 0; channel < bandBuffers[i].getNumChannels(); channel++) {
                filter.processSamples(writePtrArray[channel], bandBuffers[i].getNumSamples());
            }
        }

        setStatusMessage("Reversing bands...");
        for (int i = 0; i < 6; i++) {
            bandBuffers[i].reverse(0, bandBuffers[i].getNumSamples() - 1);
        }

        for (int i = 0; i < 6; i++) {
            setStatusMessage("Showing filtered band " + String(i+1) + "/6 (" + String(pow(2, i)*125) + " Hz)");
            audioProcessor.ir = bandBuffers[i];
            impulseResponseComponent.updateThumbnail(audioProcessor.globalSampleRate);
            sleep(1000);
        }

        audioProcessor.ir.clear();

        for (int i = 0; i < 6; i++) {
            setStatusMessage("Weighing band " + String(i+1) + "/6 (" + String(pow(2, i)*125) + " Hz)");
            auto *writePtrArray = bandBuffers[i].getArrayOfWritePointers();
            auto *readPtrArray = gainCurveBuffers[i].getArrayOfReadPointers();
            for (int channel = 0; channel < bandBuffers[i].getNumChannels(); channel++) {
                for (int sample = 0; sample < bandBuffers[i].getNumSamples(); sample++) {
                    writePtrArray[channel][sample] *= readPtrArray[channel][sample];
                }
            }
        }

        for (int i = 0; i < 6; i++) {
            setStatusMessage("Showing weighed band " + String(i+1) + "/6 (" + String(pow(2, i)*125) + " Hz)");
            audioProcessor.ir = bandBuffers[i];
            impulseResponseComponent.updateThumbnail(audioProcessor.globalSampleRate);
            sleep(1000);
        }

        buffer = bandBuffers[0];

        for (int i = 1; i < 6; i++) {
            setStatusMessage("Adding weighed bands...");
            auto *writePtrArray = buffer.getArrayOfWritePointers();
            auto *readPtrArray = bandBuffers[i].getArrayOfReadPointers();
            for (int channel = 0; channel < bandBuffers[i].getNumChannels(); channel++) {
                for (int sample = 0; sample < bandBuffers[i].getNumSamples(); sample++) {
                    writePtrArray[channel][sample] += readPtrArray[channel][sample];
                }
            }
        }

        auto *writePtrArray = buffer.getArrayOfWritePointers();
        for (int channel = 0; channel < buffer.getNumChannels(); channel++) {
            for (int sample = 0; sample < buffer.getNumSamples(); sample++) {
                writePtrArray[channel][sample] *= 1.0f;
            }
        }

        audioProcessor.ir = buffer;

        /**
        for (int i = 0; i < 6; i++) {
            auto *writePtrArray = audioProcessor.ir.getArrayOfWritePointers();
            auto *gainPtrArray = gainCurveBuffers[i].getArrayOfReadPointers();
            for (int channel = 0; channel < bandBuffers[i].getNumChannels(); channel++) {
                for (int sample = 0; sample < bandBuffers[i].getNumSamples(); sample++) {
                    writePtrArray[channel][sample] += gainPtrArray[channel][sample];
                }
            }
        }

        auto *writePtrArray = audioProcessor.ir.getArrayOfWritePointers();
        for (int channel = 0; channel < audioProcessor.ir.getNumChannels(); channel++) {
            for (int sample = 0; sample < audioProcessor.ir.getNumSamples(); sample++) {
                writePtrArray[channel][sample] *= 0.25f;
                if (randomGenerator.nextBool()) {
                    writePtrArray[channel][sample] *= -1.0f;
                }
            }
        }
        **/

        sleep(1000);
    }

    impulseResponseComponent.updateThumbnail(audioProcessor.globalSampleRate);

    sleep(1000);
}

void Raytracer::trace(Raytracer::Ray ray)
{
    SecondarySource secondarySource;

    for (int bounce = 0; bounce < maxBounces; bounce++) {
        Hit hit = calculateBounce(ray);

        if (hit.hitSurface) {
            // records secondary source
            secondarySource.order++;
            secondarySource.position = hit.hitPoint;
            secondarySource.normal = hit.normal;
            secondarySource.scatterCoefficient = hit.materialProperties.roughness;
            secondarySource.delayMS += hit.distance / speedOfSoundMpS * 1000.0f;
            secondarySource.energyCoefficients *= -hit.materialProperties.absorptionCoefficients;

            auto recordedSecondarySource = secondarySource;
            recordedSecondarySource.energyCoefficients *= hit.materialProperties.roughness;

            secondarySources.push_back(recordedSecondarySource);

            ray.position = hit.hitPoint;

            // calculate diffuse and specular portion of reflection
            glm::vec3 specularReflection = reflect(ray.direction, hit.normal);
            glm::vec3 diffuseReflection = glm::normalize(glm::vec3{randomNormalDistribution(), randomNormalDistribution(), randomNormalDistribution()});

            // dot product of normal and vector is negative if the angle between them is greater than 90 degrees
            if (dot(hit.normal, diffuseReflection) < 0) {
                // reflection would not be in the same hemisphere, so invert it
                diffuseReflection *= -1;
            }
            ray.direction = normalize(mix(specularReflection, diffuseReflection, hit.materialProperties.roughness));
            secondarySource.energyCoefficients *= 1-hit.materialProperties.roughness;
        } else {
            break;
        }
    }
}

Raytracer::Hit Raytracer::calculateBounce(Ray ray)
{
    Hit hit;

    for (WavefrontObjFile::Shape* shape : room.shapes) {

        jassert(shape->mesh.indices.size() % 3 == 0);

        for (int index = 0; index < shape->mesh.indices.size(); index += 3) {
            Triangle triangle;

            triangle.normal = shape->mesh.normals[shape->mesh.indices[index]];
            triangle.pointA = shape->mesh.vertices[shape->mesh.indices[index + 0]];
            triangle.pointB = shape->mesh.vertices[shape->mesh.indices[index + 1]];
            triangle.pointC = shape->mesh.vertices[shape->mesh.indices[index + 2]];

            Hit triangleHit = collisionTriangle(ray, triangle);

            if (triangleHit.hitSurface && triangleHit.distance < hit.distance) {
                triangleHit.materialProperties = shape->materialProperties;
                hit = triangleHit;
            }
        }
    }

    return hit;
}

/**
 * @return A random float from a normal distribution with mean 0 and standard deviation of 1
 * @see https://stackoverflow.com/a/6178290
 */
float Raytracer::randomNormalDistribution()
{
    float theta = 2 * glm::pi<float>() * randomGenerator.nextFloat();
    float rho = sqrt(-2 * log(randomGenerator.nextFloat()));

    return rho * cos(theta);
}

/**
 * Points of the ray can be expressed as
 * @code
 * P = ray.position + t*ray.direction
 * @endcode
 * with t in range from 0 to inf.
 * Points of the triangle plane can be expressed as
 * @code
 * P = pointA + u*edgeAB + v*edgeAC
 * @endcode
 * with P being inside the triangle if u+v = 1.
 *
 * @see https://en.wikipedia.org/wiki/Möller-Trumbore_intersection_algorithm
 * @see https://stackoverflow.com/a/42752998
 *
 * @param ray           Ray the intersection test is performed on. Direction vector should be normalized.
 * @param triangle      Triangle the intersection test is performed on.
 */
Raytracer::Hit Raytracer::collisionTriangle(Raytracer::Ray ray, Raytracer::Triangle triangle)
{
    Hit hit;

    if (dot(ray.direction, triangle.normal) != 0.0f) {                  // dot product is not zero: ray intersects triangle plane
        glm::vec3 edgeAB =  triangle.pointB - triangle.pointA;
        glm::vec3 edgeAC =  triangle.pointC - triangle.pointA;
        glm::vec3 n      =  cross(edgeAB, edgeAC);
        float     det    = -dot(ray.direction, n);
        glm::vec3 ap     =  ray.position - triangle.pointA;
        glm::vec3 dap    =  cross(ap, ray.direction);

        float     u      =  dot(edgeAC, dap) / det;
        float     v      = -dot(edgeAB, dap) / det;
        float     t      =  dot(ap, n)       / det;

        if (t   >= 0.0001f                                              // do not register a hit inside the same surface the rays bounces off of
         && u   >= 0.0f
         && v   >= 0.0f
         && u+v <= 1.0f) {
            hit.hitSurface = true;
            hit.hitPoint = ray.position + t * ray.direction;
            hit.distance = dot(ap, n) / det;
            hit.normal = triangle.normal;
        }
    }

    return hit;
}

/**
 * Simple visibility check that uses the geometry of the currently loaded room.
 */
bool Raytracer::checkVisibility(glm::vec3 positionA, glm::vec3 positionB)
{
    Ray edgeAB = {
            positionA,
            glm::normalize(positionB - positionA)
    };

    Hit hit = calculateBounce(edgeAB);

    if (hit.hitSurface && hit.distance < glm::length(positionB - positionA)) {
        return false;
    }

    return true;
}
