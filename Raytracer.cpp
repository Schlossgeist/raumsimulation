#include "Raytracer.h"
#include "externals/glm/glm/gtx/vector_angle.hpp"

Raytracer::Raytracer(RaumsimulationAudioProcessor& p, juce::AudioProcessorValueTreeState& pts, const String &windowTitle, bool hasProgressBar, bool hasCancelButton, int timeOutMsWhenCancelling, const String &cancelButtonText, Component *componentToCentreAround)
    : audioProcessor(p)
    , parameters(pts)
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

    {
        std::vector<Object> speakers;

        for (const auto& object : objects) {
            if (object.type == Object::Type::SPEAKER && object.active) {
                speakers.push_back(object);
            }
        }

        setStatusMessage("Casting rays...");

        for (int speakerNum = 0; speakerNum < speakers.size(); speakerNum++) {
            // user pressed "cancel"
            if (threadShouldExit())
                break;

            setStatusMessage("Casting Rays for source " + String(speakerNum + 1) + " / " + String(speakers.size()));

            for (int rayNum = 0; rayNum < raysPerSource; rayNum++) {
                // user pressed "cancel"
                if (threadShouldExit())
                    break;

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

    {
        std::vector<Object> microphones;

        for (const auto& object : objects) {
            if (object.type == Object::Type::MICROPHONE && object.active) {
                microphones.push_back(object);
                histograms.insert(std::pair{object.name, std::vector<EnergyPortion>()});
            }
        }

        setStatusMessage("Rendering...");

        for (int microphoneNum = 0; microphoneNum < microphones.size(); microphoneNum++) {
            auto microphone = microphones[microphoneNum];

            // user pressed "cancel"
            if (threadShouldExit())
                break;

            setStatusMessage("Rendering IR for receiver " + String(microphoneNum + 1) + " / " + String(microphones.size()));

            for (int secondarySourceNum = 0; secondarySourceNum < secondarySources.size(); secondarySourceNum++) {
                auto secondarySource = secondarySources[secondarySourceNum];

                // user pressed "cancel"
                if (threadShouldExit())
                    break;

                if (checkVisibility(secondarySource.position, microphone.position)) {
                    // lamberts cosine law:
                    // energy received at the observers is proportional to the cosine of the angle between the reflection vector and the surface normal
                    glm::vec3 edgeSM = glm::normalize(microphone.position - secondarySource.position);
                    float     angle  = glm::angle(glm::normalize(secondarySource.normal), edgeSM);
                    secondarySource.energy_coefficients *= cos(angle);

                    histograms.at(microphone.name).push_back({secondarySource.energy_coefficients, secondarySource.delayMS});
                }

                // update the progress bar on the dialog box
                setProgress((float) ((microphoneNum + 1) * (secondarySourceNum + 1)) / (float) (microphones.size() * secondarySources.size()));
            }
        }
    }

    setStatusMessage("Generating impulse response...");

    std::sort(histograms.at("Mic1").begin(), histograms.at("Mic1").end(), EnergyPortion::byDelay);

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
            secondarySource.energy_coefficients *= -hit.materialProperties.absorptionCoefficients;

            secondarySources.push_back(secondarySource);

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
