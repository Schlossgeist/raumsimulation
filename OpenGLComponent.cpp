/*
  ==============================================================================

    OpenGLComponent.cpp
    Created: 18 Apr 2023 9:41:30pm
    Author:  Nick

  ==============================================================================
*/

/*
  ==============================================================================

   This file is part of the JUCE examples.
   Copyright (c) 2022 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "OpenGLComponent.h"
#include "externals/glm/glm/gtx/color_space.hpp"

//==============================================================================
OpenGLComponent::OpenGLComponent(RaumsimulationAudioProcessor& p, juce::AudioProcessorValueTreeState& pts, Raytracer& r)
    : audioProcessor(p)
    , parameters(pts)
    , raytracer(r)
{
    objFileURL = static_cast<const juce::URL>(parameters.state.getProperty("obj_file_url"));
    scale = (float) parameters.state.getProperty("zoom");
    rotationSpeed = (float) parameters.state.getProperty("rotation_speed");

    setOpaque(true);
    controlsOverlay.reset(new ControlsOverlay(*this));
    addAndMakeVisible(controlsOverlay.get());


    openGLContext.setRenderer(this);
    openGLContext.setContinuousRepainting(true);
    openGLContext.attachTo(*this);
}

OpenGLComponent::~OpenGLComponent()
{
    openGLContext.detach();
}

void OpenGLComponent::paint(juce::Graphics& /*g*/)
{
}

void OpenGLComponent::resized()
{
    // This method is where you should set the bounds of any child
    // components that your component contains..

    const ScopedLock lock(mutex);

    bounds = getLocalBounds();
    controlsOverlay->setBounds(bounds);
    draggableOrientation.setViewport(bounds);
}

void OpenGLComponent::newOpenGLContextCreated()
{
}

void OpenGLComponent::openGLContextClosing()
{
}

void OpenGLComponent::renderOpenGL()
{
    using namespace ::juce::gl;

    const ScopedLock lock(mutex);

    jassert(OpenGLHelpers::isContextActive());

    auto desktopScale = (float)openGLContext.getRenderingScale();

    OpenGLHelpers::clear(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    updateShader();   // Check whether we need to compile a new shader

    if (roomShader == nullptr)
        return;

    // Having used the juce 2D renderer, it will have messed-up a whole load of GL state, so
    // we need to initialise some important settings before doing our normal GL 3D drawing..
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);

    glViewport(0, 0,
        roundToInt(desktopScale * (float)bounds.getWidth()),
        roundToInt(desktopScale * (float)bounds.getHeight()));

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);      // wireframe mode for room draw call

    roomShader->use();

    if (roomUniforms->projectionMatrix != nullptr)
        roomUniforms->projectionMatrix->setMatrix4(getProjectionMatrix().mat, 1, false);

    if (roomUniforms->viewMatrix != nullptr)
        roomUniforms->viewMatrix->setMatrix4(getViewMatrix().mat, 1, false);

    if (roomUniforms->positionMatrix != nullptr)
        roomUniforms->positionMatrix->setMatrix4(Matrix3D<float>().mat, 1, false);

    if (roomUniforms->rotationMatrix != nullptr)
        roomUniforms->rotationMatrix->setMatrix4(Matrix3D<float>().mat, 1, false);

    roomShape->draw(*roomAttributes);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    for (const auto& object : raytracer.objects) {
        if (object.type == Raytracer::Object::MICROPHONE && object.active) {
            auto microphoneMatrix = glm::translate(glm::mat4(1.0f), object.position);

            microphoneShader->use();

            if (microphoneUniforms->projectionMatrix != nullptr)
                microphoneUniforms->projectionMatrix->setMatrix4(getProjectionMatrix().mat, 1, false);

            if (microphoneUniforms->viewMatrix != nullptr)
                microphoneUniforms->viewMatrix->setMatrix4(getViewMatrix().mat, 1, false);

            if (microphoneUniforms->positionMatrix != nullptr)
                microphoneUniforms->positionMatrix->setMatrix4(glm::value_ptr(microphoneMatrix), 1, false);

            if (microphoneUniforms->rotationMatrix != nullptr)
                microphoneUniforms->rotationMatrix->setMatrix4(Matrix3D<float>().mat, 1, false);

            microphoneShape->draw(*microphoneAttributes);
        }
    }


    for (const auto& object : raytracer.objects) {
        if (object.type == Raytracer::Object::SPEAKER && object.active) {
            auto speakerMatrix = glm::translate(glm::mat4(1.0f), object.position);

            speakerShader->use();

            if (speakerUniforms->projectionMatrix != nullptr)
                speakerUniforms->projectionMatrix->setMatrix4(getProjectionMatrix().mat, 1, false);

            if (speakerUniforms->viewMatrix != nullptr)
                speakerUniforms->viewMatrix->setMatrix4(getViewMatrix().mat, 1, false);

            if (speakerUniforms->positionMatrix != nullptr)
                speakerUniforms->positionMatrix->setMatrix4(glm::value_ptr(speakerMatrix), 1 ,false);

            if (speakerUniforms->rotationMatrix != nullptr)
                speakerUniforms->rotationMatrix->setMatrix4(Matrix3D<float>().mat, 1, false);

            speakerShape->draw(*speakerAttributes);
        }
    }

    visualizationShader->use();

    if (visualizationUniforms->projectionMatrix != nullptr)
        visualizationUniforms->projectionMatrix->setMatrix4(getProjectionMatrix().mat, 1, false);

    if (visualizationUniforms->viewMatrix != nullptr)
        visualizationUniforms->viewMatrix->setMatrix4(getViewMatrix().mat, 1, false);

    if (visualizationUniforms->positionMatrix != nullptr)
        visualizationUniforms->positionMatrix->setMatrix4(Matrix3D<float>().mat, 1 ,false);

    if (visualizationUniforms->rotationMatrix != nullptr)
        visualizationUniforms->rotationMatrix->setMatrix4(Matrix3D<float>().mat, 1, false);

    struct VertexBuffer
    {
        explicit VertexBuffer(std::vector<Raytracer::SecondarySource>& secondarySources)
        {
            using namespace ::juce::gl;

            size = secondarySources.size();

            glGenBuffers(1, &vertexBuffer);
            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

            Array<OpenGLUtils::Vertex> vertices;

            for (auto secondarySource : secondarySources) {

                auto color = glm::rgbColor(glm::vec3(360.0f - (10.0f * (float) secondarySource.order), 1.0f, 0.5f));

                OpenGLUtils::Vertex vertex{
                        {secondarySource.position.x, secondarySource.position.y, secondarySource.position.z},
                        {secondarySource.normal.x, secondarySource.normal.y, secondarySource.normal.z},
                        {color.r, color.g, color.b, 0.5f},
                };

                vertices.add(vertex);
            }

            glBufferData(GL_ARRAY_BUFFER, vertices.size() * (int)sizeof(OpenGLUtils::Vertex),
                         vertices.getRawDataPointer(), GL_STATIC_DRAW);
        }

        ~VertexBuffer()
        {
            using namespace ::juce::gl;

            glDeleteBuffers(1, &vertexBuffer);
        }

        void bind()
        {
            using namespace ::juce::gl;

            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        }

        int size;
        GLuint vertexBuffer;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VertexBuffer)
    };

    VertexBuffer secondarySourcesVertexBuffer(raytracer.secondarySources);

    secondarySourcesVertexBuffer.bind();

    glPointSize(3);

    visualizationAttributes->enable();
    glDrawElements(GL_POINTS, secondarySourcesVertexBuffer.size, GL_UNSIGNED_INT, nullptr);
    visualizationAttributes->disable();


    // Reset the element buffers so child Components draw correctly
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (!controlsOverlay->isMouseButtonDownThreadsafe())
        rotation += rotationSpeed.getCurrentValue();
}

Matrix3D<float> OpenGLComponent::getProjectionMatrix() const
{
    const ScopedLock lock(mutex);

    auto w = 1.0f / (scale.getCurrentValue() + 0.1f);
    auto h = w * bounds.toFloat().getAspectRatio(false);

    return Matrix3D<float>::fromFrustum(-w, w, -h, h, 5.0f, 50.0f);
}

Matrix3D<float> OpenGLComponent::getViewMatrix() const
{
    const ScopedLock lock(mutex);

    auto viewMatrix = Matrix3D<float>::fromTranslation({ 0.0f, 0.0f, -25.0f }) * draggableOrientation.getRotationMatrix();
    auto rotationMatrix = Matrix3D<float>::rotation({ 0, rotation, 0 });

    auto flipMatrix = Matrix3D<float>(1.0f, 0.0f, 0.0f, 0.0f,       //     Y             Z
                                      0.0f, 0.0f, 1.0f, 0.0f,        // Z __|     =>      |__ Y
                                      0.0f, -1.0f, 0.0f, 0.0f,      //    /             /
                                      0.0f, 0.0f, 0.0f, 1.0f);      //   X             X

    return viewMatrix * rotationMatrix * flipMatrix;
}

void OpenGLComponent::setShaderProgram()
{
    const ScopedLock lock(shaderMutex); // Prevent concurrent access to shader strings and status

    newRoomVertexShader =
            R"(#version 450
               in vec4 position;

               uniform mat4 projectionMatrix;
               uniform mat4 viewMatrix;
               uniform mat4 positionMatrix;
               uniform mat4 rotationMatrix;

               void main()
               {
                   gl_Position = projectionMatrix * viewMatrix * positionMatrix * rotationMatrix * position;
               }
            )";
    newRoomFragmentShader =
            R"(#version 450
               layout(location = 0) out vec4 color;

               void main()
               {
                   color = vec4(0.50, 0.50, 0.50, 0.50);
               }
            )";

    newMicrophoneVertexShader =
            R"(#version 450
               in vec4 position;

               uniform mat4 projectionMatrix;
               uniform mat4 viewMatrix;
               uniform mat4 positionMatrix;
               uniform mat4 rotationMatrix;

               void main()
               {
                   gl_Position = projectionMatrix * viewMatrix * positionMatrix * rotationMatrix * position;
               }
            )";

    newMicrophoneFragmentShader =
            R"(#version 450
               layout(location = 0) out vec4 color;

               void main()
               {
                   color = vec4(0.50, 0.00, 0.00, 0.75);
               }
            )";

    newSpeakerVertexShader =
            R"(#version 450
               in vec4 position;

               uniform mat4 projectionMatrix;
               uniform mat4 viewMatrix;
               uniform mat4 positionMatrix;
               uniform mat4 rotationMatrix;

               void main()
               {
                   gl_Position = projectionMatrix * viewMatrix * positionMatrix * rotationMatrix * position;
               }
            )";

    newSpeakerFragmentShader =
            R"(#version 450
               layout(location = 0) out vec4 color;

               void main()
               {
                   color = vec4(0.00, 0.00, 0.50, 0.75);
               }
            )";

    newVisualizationVertexShader =
            R"(#version 450
               in vec4 position;
               in vec4 sourceColor;

               out vec4 destinationColor;

               uniform mat4 projectionMatrix;
               uniform mat4 viewMatrix;
               uniform mat4 positionMatrix;
               uniform mat4 rotationMatrix;

               void main()
               {
                   destinationColor = sourceColor;
                   gl_Position = projectionMatrix * viewMatrix * positionMatrix * rotationMatrix * position;
               }
            )";
    newVisualizationFragmentShader =
            R"(#version 450
               in vec4 destinationColor;

               layout(location = 0) out vec4 color;

               void main()
               {
                   color = destinationColor;
               }
            )";
}
