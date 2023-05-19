#pragma once

#include <stdexcept>

struct Band6Coefficients {
    float coefficients[6]{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,};

    float& operator[](const int index) const {
        if (0 <= index && index < 6) {
            return (float&) coefficients[index];
        } else {
            throw std::out_of_range("You tried to access an out of range coefficient. There are only 6 coefficients.");
        }
    }

    Band6Coefficients operator-() const {
        Band6Coefficients new_coefficients;

        for (int index = 0; index < 6; index++) {
            new_coefficients[index] = (1.0f - coefficients[index]);
        }
        return new_coefficients;
    }

    Band6Coefficients& operator*=(const Band6Coefficients& other) {
        for (int index = 0; index < 6; index++) {
            coefficients[index] *= other[index];
        }
        return *this;
    }

    bool operator==(const Band6Coefficients& other) {
        for (int index = 0; index < 6; index++) {
            if (coefficients[index] != other[index]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const Band6Coefficients& other) {
        for (int index = 0; index < 6; index++) {
            if (coefficients[index] != other[index]) {
                return true;
            }
        }
        return false;
    }
};

struct MaterialProperties {
    Band6Coefficients absorptionCoefficients;
    float roughness = 0.0f;
};
