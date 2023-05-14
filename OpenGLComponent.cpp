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

//==============================================================================
OpenGLComponent::OpenGLComponent(RaumsimulationAudioProcessor& p, juce::AudioProcessorValueTreeState& pts)
    : audioProcessor(p)
    , parameters(pts)
{
    objFileURL = static_cast<const juce::URL>(parameters.state.getProperty("obj_file_url"));

    setOpaque(true);
    controlsOverlay.reset(new ControlsOverlay(*this));
    addAndMakeVisible(controlsOverlay.get());


    openGLContext.setRenderer(this);
    openGLContext.setContinuousRepainting(true);
    openGLContext.attachTo(*this);

    controlsOverlay->initialise();
}

OpenGLComponent::~OpenGLComponent()
{
    openGLContext.detach();
}

void OpenGLComponent::paint (juce::Graphics& /*g*/)
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

    OpenGLHelpers::clear(Colours::transparentBlack);

    updateShader();   // Check whether we need to compile a new shader

    if (shader.get() == nullptr)
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

    auto microphoneMatrix = Matrix3D<float>(1.0f, 0.0f, 0.0f, 0.0f,
                                           0.0f, 1.0f, 0.0f, 0.0f,
                                           0.0f, 0.0f, 1.0f, 0.0f,
                                           0.0f, 0.0f, 0.75f, 1.0f);

    auto speakerMatrix = Matrix3D<float>(1.0f, 0.0f, 0.0f, 0.0f,
                                         0.0f, 1.0f, 0.0f, 0.0f,
                                         0.0f, 0.0f, 1.0f, 0.0f,
                                         0.5f, 0.5f, 0.75f, 1.0f);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);      // wireframe mode for room draw call

    shader->use();

    if (uniforms->projectionMatrix != nullptr)
        uniforms->projectionMatrix->setMatrix4(getProjectionMatrix().mat, 1, false);

    if (uniforms->viewMatrix != nullptr)
        uniforms->viewMatrix->setMatrix4(getViewMatrix().mat, 1, false);

    if (uniforms->positionMatrix != nullptr)
        uniforms->positionMatrix->setMatrix4(Matrix3D<float>().mat, 1, false);

    if (uniforms->rotationMatrix != nullptr)
        uniforms->rotationMatrix->setMatrix4(Matrix3D<float>().mat, 1, false);

    shape->draw(*attributes);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    microphoneShader->use();

    if (uniforms->projectionMatrix != nullptr)
        uniforms->projectionMatrix->setMatrix4(getProjectionMatrix().mat, 1, false);

    if (uniforms->viewMatrix != nullptr)
        uniforms->viewMatrix->setMatrix4(getViewMatrix().mat, 1, false);

    if (uniforms->positionMatrix != nullptr)
        uniforms->positionMatrix->setMatrix4(microphoneMatrix.mat, 1, false);

    if (uniforms->rotationMatrix != nullptr)
        uniforms->rotationMatrix->setMatrix4(Matrix3D<float>().mat, 1, false);

    microphoneShape->draw(*attributes);

    speakerShader->use();

    if (uniforms->projectionMatrix != nullptr)
        uniforms->projectionMatrix->setMatrix4(getProjectionMatrix().mat, 1, false);

    if (uniforms->viewMatrix != nullptr)
        uniforms->viewMatrix->setMatrix4(getViewMatrix().mat, 1, false);

    if (uniforms->positionMatrix != nullptr)
        uniforms->positionMatrix->setMatrix4(speakerMatrix.mat, 1 ,false);

    if (uniforms->rotationMatrix != nullptr)
        uniforms->rotationMatrix->setMatrix4(Matrix3D<float>().mat, 1, false);

    speakerShape->draw(*attributes);

    // Reset the element buffers so child Components draw correctly
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    if (!controlsOverlay->isMouseButtonDownThreadsafe())
        rotation += rotationSpeed.getCurrentValue();
}

Matrix3D<float> OpenGLComponent::getProjectionMatrix() const
{
    const ScopedLock lock(mutex);

    auto w = 1.0f / (scale.getCurrentValue() * 2.0f + 0.1f);
    auto h = w * bounds.toFloat().getAspectRatio(false);

    return Matrix3D<float>::fromFrustum(-w, w, -h, h, 4.0f, 30.0f);
}

Matrix3D<float> OpenGLComponent::getViewMatrix() const
{
    const ScopedLock lock(mutex);

    auto viewMatrix = Matrix3D<float>::fromTranslation({ 0.0f, 0.0f, -10.0f }) * draggableOrientation.getRotationMatrix();
    auto rotationMatrix = Matrix3D<float>::rotation({ 0, rotation, 0 });

    auto flipMatrix = Matrix3D<float>(1.0f, 0.0f, 0.0f, 0.0f,       //     Y             Z
                                      0.0f, 0.0f, 1.0f, 0.0f,        // Z __|     =>      |__ Y
                                      0.0f, -1.0f, 0.0f, 0.0f,      //    /             /
                                      0.0f, 0.0f, 0.0f, 1.0f);      //   X             X

    return viewMatrix * rotationMatrix * flipMatrix;
}

void OpenGLComponent::setShaderProgram(const String& vertexShader, const String& fragmentShader)
{
    const ScopedLock lock(shaderMutex); // Prevent concurrent access to shader strings and status

    OpenGLUtils::ShaderPreset roomPreset =
    {
        "Room Shader",

        SHADER_DEMO_HEADER
        "in vec4 position;\n"
        "in vec4 sourceColour;\n"
        "in vec2 textureCoordIn;\n"
        "\n"
        "uniform mat4 projectionMatrix;\n"
        "uniform mat4 viewMatrix;\n"
        "\n"
        "out vec4 destinationColour;\n"
        "out vec2 textureCoordOut;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    destinationColour = sourceColour;\n"
        "    textureCoordOut = textureCoordIn;\n"
        "    gl_Position = projectionMatrix * viewMatrix * position;\n"
        "}\n",

        SHADER_DEMO_HEADER
        "varying vec4 destinationColour;\n"
        "varying vec2 textureCoordOut;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   vec4 colour = vec4(0.50, 0.50, 0.50, 0.50);\n"
        "   gl_FragColor = colour;\n"
        "}\n"
    };

    OpenGLUtils::ShaderPreset microphonePreset =
    {
        "Microphone Shader",

        SHADER_DEMO_HEADER
        "in vec4 position;\n"
        "in vec4 sourceColour;\n"
        "in vec2 textureCoordIn;\n"
        "\n"
        "uniform mat4 projectionMatrix;\n"
        "uniform mat4 viewMatrix;\n"
        "uniform mat4 positionMatrix;\n"
        "uniform mat4 rotationMatrix;\n"
        "\n"
        "out vec4 destinationColour;\n"
        "out vec2 textureCoordOut;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    destinationColour = sourceColour;\n"
        "    textureCoordOut = textureCoordIn;\n"
        "    gl_Position = projectionMatrix * viewMatrix * positionMatrix * position;\n"
        "}\n",

        SHADER_DEMO_HEADER
        "out vec4 destinationColour;\n"
        "out vec2 textureCoordOut;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   vec4 colour = vec4(0.50, 0.00, 0.00, 0.75);\n"
        "   gl_FragColor = colour;\n"
        "}\n"
    };

    OpenGLUtils::ShaderPreset speakerPreset =
    {
        "Speaker Shader",

        SHADER_DEMO_HEADER
        "in vec4 position;\n"
        "in vec4 sourceColour;\n"
        "in vec2 textureCoordIn;\n"
        "\n"
        "uniform mat4 projectionMatrix;\n"
        "uniform mat4 viewMatrix;\n"
        "uniform mat4 positionMatrix;\n"
        "uniform mat4 rotationMatrix;\n"
        "\n"
        "out vec4 destinationColour;\n"
        "out vec2 textureCoordOut;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    destinationColour = sourceColour;\n"
        "    textureCoordOut = textureCoordIn;\n"
        "    gl_Position = projectionMatrix * viewMatrix * positionMatrix * position;\n"
        "}\n",

        SHADER_DEMO_HEADER
        "out vec4 destinationColour;\n"
        "out vec2 textureCoordOut;\n"
        "\n"
        "void main()\n"
        "{\n"
        "   vec4 colour = vec4(0.00, 0.00, 0.50, 0.75);\n"
        "   gl_FragColor = colour;\n"
        "}\n"
    };

    newVertexShader = roomPreset.vertexShader;
    newFragmentShader = roomPreset.fragmentShader;

    newMicrophoneVertexShader = microphonePreset.vertexShader;
    newMicrophoneFragmentShader = microphonePreset.fragmentShader;

    newSpeakerVertexShader = speakerPreset.vertexShader;
    newSpeakerFragmentShader = speakerPreset.fragmentShader;
}