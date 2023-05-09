# pragma once

#include <JuceHeader.h>
#include "WavefrontObjParser.h"

class Raytracer {
public:
    Raytracer() = default;
    ~Raytracer() = default;

    void setRoom(const File& objFile);
private:
    WavefrontObjFile room;

};
