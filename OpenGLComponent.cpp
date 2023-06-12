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
#include <glm/gtx/color_space.hpp>
#include <memory>

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
    controlsOverlay = std::make_unique<ControlsOverlay>(*this);
    addAndMakeVisible(controlsOverlay.get());


    openGLContext.setRenderer(this);
    openGLContext.setContinuousRepainting(true);

    OpenGLPixelFormat pixelFormat;
    pixelFormat.multisamplingLevel = 16;
    openGLContext.setPixelFormat(pixelFormat);
    openGLContext.setMultisamplingEnabled(true);
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
    const ScopedLock lock(shaderMutex);

    jassert(OpenGLHelpers::isContextActive());

    String rmvpVertexShaderString =
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

    String cFragmentShaderString =
            R"(#version 450
               in vec4 destinationColor;

               layout(location = 0) out vec4 color;

               void main()
               {
                   color = destinationColor;
               }
            )";

    String roomFragmentShaderString =
            R"(#version 450
               in vec4 destinationColor;

               layout(location = 0) out vec4 color;

               void main()
               {
                   color = vec4(0.50, 0.50, 0.50, 0.50);
               }
            )";

    String microphoneFragmentShaderString =
            R"(#version 450
               in vec4 destinationColor;

               layout(location = 0) out vec4 color;

               void main()
               {
                   color = vec4(0.50, 0.00, 0.00, 0.75);
               }
            )";

    String speakerFragmentShaderString =
            R"(#version 450
               in vec4 destinationColor;

               layout(location = 0) out vec4 color;

               void main()
               {
                   color = vec4(0.00, 0.00, 0.50, 0.75);
               }
            )";

    genericShader = std::make_unique<Shader>(rmvpVertexShaderString, cFragmentShaderString, openGLContext);
    roomRRRShader = std::make_unique<Shader>(rmvpVertexShaderString, roomFragmentShaderString, openGLContext);
    microphoneRRRShader = std::make_unique<Shader>(rmvpVertexShaderString, microphoneFragmentShaderString, openGLContext);
    speakerRRRShader = std::make_unique<Shader>(rmvpVertexShaderString, speakerFragmentShaderString, openGLContext);

    updateRoomModel();

    roomAttributes = std::make_shared<OpenGLUtils::Attributes>(*roomRRRShader->getShaderProgram());
    visualizationAttributes = std::make_shared<OpenGLUtils::Attributes>(*genericShader->getShaderProgram());
    coordAttributes = std::make_shared<OpenGLUtils::Attributes>(*genericShader->getShaderProgram());
    floodAttributes = std::make_shared<OpenGLUtils::Attributes>(*genericShader->getShaderProgram());

    auto headFileStream = std::make_unique<MemoryInputStream>(BinaryData::head_obj, BinaryData::head_objSize, true);
    microphoneShape = std::make_shared<OpenGLUtils::Shape>(String(CharPointer_UTF8((const char*) headFileStream->getData())));
    microphoneAttributes = std::make_shared<OpenGLUtils::Attributes>(*microphoneRRRShader->getShaderProgram());

    auto ballFileStream = std::make_unique<MemoryInputStream>(BinaryData::ball_obj, BinaryData::ball_objSize, true);
    speakerShape = std::make_shared<OpenGLUtils::Shape>(String(CharPointer_UTF8((const char*) ballFileStream->getData())));
    speakerAttributes = std::make_shared<OpenGLUtils::Attributes>(*speakerRRRShader->getShaderProgram());
}

void OpenGLComponent::openGLContextClosing()
{
}

void OpenGLComponent::renderOpenGL()
{
    using namespace ::juce::gl;

    const ScopedLock lock(mutex);

    jassert(OpenGLHelpers::isContextActive());

    auto desktopScale = (float) openGLContext.getRenderingScale();

    OpenGLHelpers::clear(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

    updateRoomModel();

    if (roomRRRShader == nullptr)
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

    // ROOM
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);      // wireframe mode for room draw call

    roomRRRShader->bind();

    roomRRRShader->setUniformMatrix4("projectionMatrix", getProjectionMatrix().mat, 1, false);
    roomRRRShader->setUniformMatrix4("viewMatrix", getViewMatrix().mat, 1, false);
    roomRRRShader->setUniformMatrix4("positionMatrix", Matrix3D<float>().mat, 1 ,false);
    roomRRRShader->setUniformMatrix4("rotationMatrix", Matrix3D<float>().mat, 1, false);

    roomShape->draw(*roomAttributes);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // MICROPHONES
    for (const auto& object : raytracer.objects) {
        if (object.type == Raytracer::Object::MICROPHONE && object.active) {
            auto microphoneMatrix = glm::translate(glm::mat4(1.0f), object.position);

            microphoneRRRShader->bind();

            microphoneRRRShader->setUniformMatrix4("projectionMatrix", getProjectionMatrix().mat, 1, false);
            microphoneRRRShader->setUniformMatrix4("viewMatrix", getViewMatrix().mat, 1, false);
            microphoneRRRShader->setUniformMatrix4("positionMatrix", glm::value_ptr(microphoneMatrix), 1 ,false);
            microphoneRRRShader->setUniformMatrix4("rotationMatrix", Matrix3D<float>().mat, 1, false);

            microphoneShape->draw(*microphoneAttributes);
        }
    }

    // SPEAKERS
    for (const auto& object : raytracer.objects) {
        if (object.type == Raytracer::Object::SPEAKER && object.active) {
            auto speakerMatrix = glm::translate(glm::mat4(1.0f), object.position);

            speakerRRRShader->bind();

            speakerRRRShader->setUniformMatrix4("projectionMatrix", getProjectionMatrix().mat, 1, false);
            speakerRRRShader->setUniformMatrix4("viewMatrix", getViewMatrix().mat, 1, false);
            speakerRRRShader->setUniformMatrix4("positionMatrix", glm::value_ptr(speakerMatrix), 1 ,false);
            speakerRRRShader->setUniformMatrix4("rotationMatrix", Matrix3D<float>().mat, 1, false);

            speakerShape->draw(*speakerAttributes);
        }
    }

    updateVisualizationVertexBuffers();

    // VISUALIZATION
    genericShader->bind();

    genericShader->setUniformMatrix4("projectionMatrix", getProjectionMatrix().mat, 1, false);
    genericShader->setUniformMatrix4("viewMatrix", getViewMatrix().mat, 1, false);
    genericShader->setUniformMatrix4("positionMatrix", Matrix3D<float>().mat, 1 ,false);
    genericShader->setUniformMatrix4("rotationMatrix", Matrix3D<float>().mat, 1, false);

    VertexBuffer secondarySourcesVertexBuffer(visualizationVertices.getRawDataPointer(), sizeof(OpenGLUtils::Vertex) * visualizationVertices.size());

    secondarySourcesVertexBuffer.bind();

    glPointSize(3);

    visualizationAttributes->enable();
    glDrawElements(GL_POINTS, visualizationVertices.size(), GL_UNSIGNED_INT, nullptr);
    visualizationAttributes->disable();

    // FLOOD FILL
    genericShader->bind();

    genericShader->setUniformMatrix4("projectionMatrix", getProjectionMatrix().mat, 1, false);
    genericShader->setUniformMatrix4("viewMatrix", getViewMatrix().mat, 1, false);
    genericShader->setUniformMatrix4("positionMatrix", Matrix3D<float>().mat, 1 ,false);
    genericShader->setUniformMatrix4("rotationMatrix", Matrix3D<float>().mat, 1, false);

    VertexBuffer cubesVertexBuffer(floodVertices.getRawDataPointer(), sizeof(OpenGLUtils::Vertex) * floodVertices.size());

    cubesVertexBuffer.bind();

    glPointSize(3);

    floodAttributes->enable();
    glDrawElements(GL_POINTS, floodVertices.size(), GL_UNSIGNED_INT, nullptr);
    floodAttributes->disable();

    // COORDINATE AXIS
    glViewport(0, 0,
               roundToInt(desktopScale * (float)bounds.getWidth() / 10),
               roundToInt(desktopScale * (float)bounds.getHeight() / 10));

    genericShader->bind();

    genericShader->setUniformMatrix4("projectionMatrix", getCoordProjectionMatrix().mat, 1, false);
    genericShader->setUniformMatrix4("viewMatrix", getViewMatrix().mat, 1, false);
    genericShader->setUniformMatrix4("positionMatrix", Matrix3D<float>().mat, 1 ,false);
    genericShader->setUniformMatrix4("rotationMatrix", Matrix3D<float>().mat, 1, false);

    OpenGLUtils::Vertex oVertex{{0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 0.0f},
    };

    OpenGLUtils::Vertex xVertex{{1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f, 1.0f},
    };

    OpenGLUtils::Vertex yVertex{{0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 1.0f},
    };

    OpenGLUtils::Vertex zVertex{{0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {0.0f, 0.0f, 1.0f, 1.0f},
    };

    Array<OpenGLUtils::Vertex> coordVertices = {oVertex, xVertex, yVertex, zVertex};

    unsigned int indices[6] = {
            0, 1,
            0, 2,
            0, 3
    };

    VertexBuffer coordVertexBuffer(coordVertices.getRawDataPointer(), sizeof(OpenGLUtils::Vertex) * coordVertices.size());
    IndexBuffer coordIndexBuffer(indices, 6);

    coordVertexBuffer.bind();
    coordIndexBuffer.bind();

    coordAttributes->enable();
    glDrawElements(GL_LINES, coordIndexBuffer.getCount(), GL_UNSIGNED_INT, nullptr);
    coordAttributes->disable();

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

Matrix3D<float> OpenGLComponent::getCoordProjectionMatrix() const
{
    const ScopedLock lock(mutex);

    auto w = 0.25f;
    auto h = w * bounds.toFloat().getAspectRatio(false);

    return Matrix3D<float>::fromFrustum(-w, w, -h, h, 5.0f, 50.0f);
}
