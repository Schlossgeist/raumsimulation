#pragma once

#include <stdexcept>

struct AbsorptionCoefficients {
    float* absorption_coefficients = new float[6]{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,};

    float& operator[](const int index) const {
        if (0 <= index && index < 6) {
            return absorption_coefficients[index];
        } else {
            throw std::out_of_range("You tried to access an out of range absorption coefficient. There are only 6 coefficients.");
        }
    }

    AbsorptionCoefficients operator-() const {
        AbsorptionCoefficients coefficients;

        for (int index = 0; index < 6; index++) {
            coefficients[index] = (1 - absorption_coefficients[index]);
            index++;
        }
        return coefficients;
    }

    AbsorptionCoefficients& operator*=(const AbsorptionCoefficients& other) {
        for (int index = 0; index < 6; index++) {
            absorption_coefficients[index] *= other[index];
            index++;
        }
        return *this;
    }
};
