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
OpenGLComponent::OpenGLComponent(RaumsimulationAudioProcessor& p, juce::AudioProcessorValueTreeState& pts, Raytracer& r)
    : audioProcessor(p)
    , parameters(pts)
    , raytracer(r)
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

    for (const auto& objectPair : raytracer.objects) {
        auto microphone = objectPair.second;
        if (microphone.type == Raytracer::Object::MICROPHONE && microphone.active) {
            auto microphoneMatrix = Matrix3D<float>(1.0f, 0.0f, 0.0f, 0.0f,
                                                    0.0f, 1.0f, 0.0f, 0.0f,
                                                    0.0f, 0.0f, 1.0f, 0.0f,
                                                    microphone.position.x, microphone.position.y, microphone.position.z, 1.0f);

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
        }
    }


    for (const auto& objectPair : raytracer.objects) {
        auto speaker = objectPair.second;
        if (speaker.type == Raytracer::Object::SPEAKER && speaker.active) {
            auto speakerMatrix = Matrix3D<float>(1.0f, 0.0f, 0.0f, 0.0f,
                                                    0.0f, 1.0f, 0.0f, 0.0f,
                                                    0.0f, 0.0f, 1.0f, 0.0f,
                                                 speaker.position.x, speaker.position.y, speaker.position.z, 1.0f);

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
        }
    }

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

void OpenGLComponent::setShaderProgram(const String& vertexShader, const String& fragmentShader)
{
    const ScopedLock lock(shaderMutex); // Prevent concurrent access to shader strings and status

    auto roomShaderStream = new MemoryInputStream(BinaryData::room_shader, BinaryData::room_shaderSize, true);
    auto microphoneShaderStream = new MemoryInputStream(BinaryData::microphone_shader, BinaryData::microphone_shaderSize, true);
    auto speakerShaderStream = new MemoryInputStream(BinaryData::speaker_shader, BinaryData::speaker_shaderSize, true);


    newVertexShader = parseShader(*roomShaderStream, SHADER_VERTEX);
    newFragmentShader = parseShader(*roomShaderStream, SHADER_FRAGMENT);

    newMicrophoneVertexShader = parseShader(*microphoneShaderStream, SHADER_VERTEX);
    newMicrophoneFragmentShader = parseShader(*microphoneShaderStream, SHADER_FRAGMENT);

    newSpeakerVertexShader = parseShader(*speakerShaderStream, SHADER_VERTEX);
    newSpeakerFragmentShader = parseShader(*speakerShaderStream, SHADER_FRAGMENT);
}

static bool matchToken (String::CharPointerType& t, const char* token)
{
    auto len = (int) strlen (token);

    if (CharacterFunctions::compareUpTo (CharPointer_ASCII (token), t, len) == 0)
    {
        auto end = t + len;

        if (end.isEmpty() || end.isWhitespace())
        {
            t = end.findEndOfWhitespace();
            return true;
        }
    }

    return false;
}

String OpenGLComponent::parseShader(const MemoryInputStream& shader_stream, ShaderType shaderToExtract)
{
    ShaderType currentShaderType = SHADER_NONE;
    String result;
    StringArray lines = StringArray::fromLines(String(CharPointer_UTF8((const char*) shader_stream.getData())));

    for (auto lineNum = 0; lineNum < lines.size() - 1; ++lineNum) {
        auto l = lines[lineNum].getCharPointer().findEndOfWhitespace();

        if (matchToken (l, "#SHADER_VERTEX"))       { currentShaderType = SHADER_VERTEX;       continue; }
        if (matchToken (l, "#SHADER_FRAGMENT"))     { currentShaderType = SHADER_FRAGMENT;     continue; }
        if (matchToken (l, "#SHADER_COMPUTE"))      { currentShaderType = SHADER_COMPUTE;      continue; }

        if (shaderToExtract == currentShaderType) {
            result += String(l).trim();
            result += "\n";
        }
    }

    return result;
}
