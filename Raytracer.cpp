#include "Raytracer.h"

Raytracer::Raytracer(RaumsimulationAudioProcessor& p, juce::AudioProcessorValueTreeState& pts, const String &windowTitle, bool hasProgressBar, bool hasCancelButton, int timeOutMsWhenCancelling, const String &cancelButtonText, Component *componentToCentreAround)
    : audioProcessor(p)
    , parameters(pts)
    , ThreadWithProgressWindow(windowTitle, hasProgressBar, hasCancelButton, timeOutMsWhenCancelling, cancelButtonText, componentToCentreAround)
{
    microphones.push_back(Microphone{true, glm::vec3{2.5f, 3.5f, 2.0f}});
    speakers.push_back(Speaker{true, glm::vec3{7.0f, -1.0f, 3.0f}});

    //microphones.push_back(Microphone{true, glm::vec3{1.0f, 0.5f, 2.0f}});
    //speakers.push_back(Speaker{true, glm::vec3{3.0f, 1.0f, 3.0f}});
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

    setStatusMessage("Generating impulse response...");
    sleep(1000);
}

void Raytracer::trace(Raytracer::Ray ray)
{
    Reflection reflection;

    for (int bounce = 0; bounce < maxBounces; bounce++) {
        Hit hit = calculateBounce(ray, reflection);

        if (hit.surface) {
            reflection.delayMS += hit.distance / speedOfSoundMpS * 1000.0f;

            reflection.energy_coefficients *= -hit.materialProperties.absorptionCoefficients;

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

Raytracer::Hit Raytracer::calculateBounce(Ray ray, Reflection reflection)
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

            if (triangleHit.surface && triangleHit.distance < hit.distance) {
                triangleHit.materialProperties = shape->materialProperties;
                hit = triangleHit;
            }
        }
    }

    for (Microphone& microphone : microphones) {
        Hit sphereHit = collisionSphere(ray, microphone.position, 0.5f);

        if (sphereHit.surface && sphereHit.distance < hit.distance && microphone.active) {
            reflection.delayMS += sphereHit.distance / speedOfSoundMpS * 1000.0f;
            microphone.registeredReflections.push_back(reflection);
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
 * Standard quadratic equation with
 * @code
 * p = 2*(ray.direction ⋅ (ray.position - center))/(ray.direction)^2
 * q = ((ray.position - center)^2 - radius^2)/(ray.direction)^2
 * @endcode
 *
 * @see https://en.wikipedia.org/wiki/Line-sphere_intersection
 */
Raytracer::Hit Raytracer::collisionSphere(Raytracer::Ray ray, glm::vec3 center, float radius)
{
    Hit hit;

    float p        = 2*dot(ray.direction, ray.position - center)/dot(ray.direction, ray.direction);
    float q        = (dot(ray.position - center, ray.position - center) - radius * radius)/dot(ray.direction, ray.direction);
    float radicand = p*p/4 - q;

    if (radicand >= 0) {                                                // equation has solution(s): ray intersects the sphere in 1 or 2 points
        float distance = -p/2 - sqrt(radicand);                     // only calculate nearest intersection

        if (distance >= 0) {                                            // ignore intersections behind the ray
            hit.surface = Hit::RECEIVER;
            hit.distance = distance;
            hit.hitPoint = ray.position + (ray.direction * distance);   // move direction to origin and scale by distance
            hit.normal = normalize(hit.hitPoint - center);
        } else {
            hit.surface = Hit::NONE;
        }
    } else {                // equation has no solution: ray missed the sphere
        hit.surface = Hit::NONE;
    }

    return hit;
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

        if (t   >= 0.0f
         && u   >= 0.0f
         && v   >= 0.0f
         && u+v <= 1.0f) {
            hit.surface = Hit::WALL;
            hit.hitPoint = ray.position + t * ray.direction;
            hit.distance = dot(ap, n) / det;
            hit.normal = triangle.normal;
        } else {
            hit.surface = Hit::NONE;
        }
    } else {                                                            // dot product is zero when ray and triangle plane are parallel: no intersection
        hit.surface = Hit::NONE;
    }

    return hit;
}
