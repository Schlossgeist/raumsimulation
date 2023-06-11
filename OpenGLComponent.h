#pragma once

#include "OpenGLUtility.h"
#include "PluginProcessor.h"
#include "Raytracer.h"
#include "WavefrontObjParser.h"
#include <glm/glm.hpp>
#include <JuceHeader.h>

struct OpenGLUtils
{
    struct Vertex
    {
        float position[3];
        float normal[3];
        float color[4];
    };

    struct Attributes
    {
        explicit Attributes(OpenGLShaderProgram& shader)
        {
            position.reset(createAttribute(shader, "position"));
            sourceColor.reset(createAttribute(shader, "sourceColor"));
        }

        void enable()
        {
            using namespace ::juce::gl;

            if (position != nullptr)
            {
                glVertexAttribPointer(position->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
                glEnableVertexAttribArray(position->attributeID);
            }

            if (normal != nullptr)
            {
                glVertexAttribPointer(normal->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(sizeof(float) * 3));
                glEnableVertexAttribArray(normal->attributeID);
            }

            if (sourceColor != nullptr)
            {
                glVertexAttribPointer(sourceColor->attributeID, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(sizeof(float) * 6));
                glEnableVertexAttribArray(sourceColor->attributeID);
            }
        }

        void disable()
        {
            using namespace ::juce::gl;

            if (position != nullptr)        glDisableVertexAttribArray(position->attributeID);
            if (normal != nullptr)          glDisableVertexAttribArray(normal->attributeID);
            if (sourceColor != nullptr)    glDisableVertexAttribArray(sourceColor->attributeID);
        }

        std::unique_ptr<OpenGLShaderProgram::Attribute> position, normal, sourceColor;

    private:
        static OpenGLShaderProgram::Attribute* createAttribute(OpenGLShaderProgram& shader,
            const char* attributeName)
        {
            using namespace ::juce::gl;

            if (glGetAttribLocation(shader.getProgramID(), attributeName) < 0)
                return nullptr;

            return new OpenGLShaderProgram::Attribute(shader, attributeName);
        }
    };

    struct Uniforms
    {
        explicit Uniforms(OpenGLShaderProgram& shader)
        {
            projectionMatrix.reset(createUniform(shader, "projectionMatrix"));
            viewMatrix.reset(createUniform(shader, "viewMatrix"));
            positionMatrix.reset(createUniform(shader, "positionMatrix"));
            rotationMatrix.reset(createUniform(shader, "rotationMatrix"));
        }

        std::unique_ptr<OpenGLShaderProgram::Uniform> projectionMatrix, viewMatrix, positionMatrix, rotationMatrix;

    private:
        static OpenGLShaderProgram::Uniform* createUniform(OpenGLShaderProgram& shader,
            const char* uniformName)
        {
            using namespace ::juce::gl;

            if (glGetUniformLocation(shader.getProgramID(), uniformName) < 0)
                return nullptr;

            return new OpenGLShaderProgram::Uniform(shader, uniformName);
        }
    };

    struct Shape
    {
        Shape()
        {
            auto dir = juce::File::getCurrentWorkingDirectory();

            int numTries = 0;

            while (!dir.getChildFile("Resources").exists() && numTries++ < 15)
                dir = dir.getParentDirectory();

            if (shapeFile.load(dir.getChildFile("Resources").getChildFile("raum001.obj")).wasOk())
                for (auto* s : shapeFile.shapes)
                    vertexBuffers.add(new VertexBuffer(*s));
        }

        explicit Shape(const File& fileToLoad)
        {
            if (shapeFile.load(fileToLoad).wasOk())
                for (auto* s : shapeFile.shapes)
                    vertexBuffers.add(new VertexBuffer(*s));
        }

        explicit Shape(const String& fileToLoad)
        {
            if (shapeFile.load(fileToLoad).wasOk())
                for (auto* s : shapeFile.shapes)
                    vertexBuffers.add(new VertexBuffer(*s));
        }

        void draw(Attributes& attributes)
        {
            using namespace ::juce::gl;

            for (auto* vertexBuffer : vertexBuffers)
            {
                vertexBuffer->bind();

                attributes.enable();
                glDrawElements(GL_TRIANGLES, vertexBuffer->numIndices, GL_UNSIGNED_INT, nullptr);
                attributes.disable();
            }
        }

    private:
        struct VertexBuffer
        {
            explicit VertexBuffer(WavefrontObjFile::Shape& shape)
            {
                using namespace ::juce::gl;

                numIndices = shape.mesh.indices.size();

                glGenBuffers(1, &vertexBuffer);
                glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

                Array<Vertex> vertices;
                createVertexListFromMesh(shape.mesh, vertices, Colours::green);

                glBufferData(GL_ARRAY_BUFFER, vertices.size() * (int)sizeof(Vertex),
                    vertices.getRawDataPointer(), GL_STATIC_DRAW);

                glGenBuffers(1, &indexBuffer);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * (int)sizeof(juce::uint32),
                    shape.mesh.indices.data(), GL_STATIC_DRAW);
            }

            ~VertexBuffer()
            {
                using namespace ::juce::gl;

                glDeleteBuffers(1, &vertexBuffer);
                glDeleteBuffers(1, &indexBuffer);
            }

            void bind()
            {
                using namespace ::juce::gl;

                glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            }

            GLuint vertexBuffer, indexBuffer;
            int numIndices;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VertexBuffer)
        };

        WavefrontObjFile shapeFile;
        OwnedArray<VertexBuffer> vertexBuffers;

        static void createVertexListFromMesh(const WavefrontObjFile::Mesh& mesh, Array<Vertex>& list, Colour colour)
        {
            auto scale = 1.0f;
            glm::vec3 defaultNormal = { 0.5f, 0.5f, 0.5f };

            for (int i = 0; i < mesh.vertices.size(); ++i)
            {
                auto& v = mesh.vertices.at(i);

                auto& n = (i < mesh.normals.size() ? mesh.normals.at(i)
                    : defaultNormal);

                list.add({ { scale * v.x, scale * v.y, scale * v.z, },
                            { scale * n.x, scale * n.y, scale * n.z, },
                            { colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha() } });
            }
        }
    };

    //==============================================================================
    struct ShaderPreset
    {
        const char* name;
        const char* vertexShader;
        const char* fragmentShader;
    };

    static Array<ShaderPreset> getPresets()
    {
#define SHADER_DEMO_HEADER \
            "/*  This is a live OpenGL Shader demo.\n" \
            "    Edit the shader program below and it will be \n" \
            "    compiled and applied to the model above!\n" \
            "*/\n\n"

        ShaderPreset presets[] =
        {
            {
                "Flat Colour",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec4 sourceColour;\n"
                "attribute vec2 textureCoordIn;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "\n"
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    destinationColour = sourceColour;\n"
                "    textureCoordOut = textureCoordIn;\n"
                "    gl_Position = projectionMatrix * viewMatrix * position;\n"
                "}\n",

                SHADER_DEMO_HEADER
               #if JUCE_OPENGL_ES
                "varying lowp vec4 destinationColour;\n"
                "varying lowp vec2 textureCoordOut;\n"
               #else
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
               #endif
                "\n"
                "void main()\n"
                "{\n"
                "    gl_FragColor = destinationColour;\n"
                "}\n"
            },

            {
                "Rainbow",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec4 sourceColour;\n"
                "attribute vec2 textureCoordIn;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "\n"
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
                "\n"
                "varying float xPos;\n"
                "varying float yPos;\n"
                "varying float zPos;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    vec4 v = vec4 (position);\n"
                "    xPos = clamp (v.x, 0.0, 1.0);\n"
                "    yPos = clamp (v.y, 0.0, 1.0);\n"
                "    zPos = clamp (v.z, 0.0, 1.0);\n"
                "    gl_Position = projectionMatrix * viewMatrix * position;\n"
                "}",

                SHADER_DEMO_HEADER
               #if JUCE_OPENGL_ES
                "varying lowp vec4 destinationColour;\n"
                "varying lowp vec2 textureCoordOut;\n"
                "varying lowp float xPos;\n"
                "varying lowp float yPos;\n"
                "varying lowp float zPos;\n"
               #else
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
                "varying float xPos;\n"
                "varying float yPos;\n"
                "varying float zPos;\n"
               #endif
                "\n"
                "void main()\n"
                "{\n"
                "    gl_FragColor = vec4 (xPos, yPos, zPos, 1.0);\n"
                "}"
            },

            {
                "Changing Colour",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec2 textureCoordIn;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "\n"
                "varying vec2 textureCoordOut;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    textureCoordOut = textureCoordIn;\n"
                "    gl_Position = projectionMatrix * viewMatrix * position;\n"
                "}\n",

                SHADER_DEMO_HEADER
                "#define PI 3.1415926535897932384626433832795\n"
                "\n"
               #if JUCE_OPENGL_ES
                "precision mediump float;\n"
                "varying lowp vec2 textureCoordOut;\n"
               #else
                "varying vec2 textureCoordOut;\n"
               #endif
                "uniform float bouncingNumber;\n"
                "\n"
                "void main()\n"
                "{\n"
                "   float b = bouncingNumber;\n"
                "   float n = b * PI * 2.0;\n"
                "   float sn = (sin (n * textureCoordOut.x) * 0.5) + 0.5;\n"
                "   float cn = (sin (n * textureCoordOut.y) * 0.5) + 0.5;\n"
                "\n"
                "   vec4 col = vec4 (b, sn, cn, 1.0);\n"
                "   gl_FragColor = col;\n"
                "}\n"
            },

            {
                "Simple Light",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec4 normal;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "uniform vec4 lightPosition;\n"
                "\n"
                "varying float lightIntensity;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    vec4 light = viewMatrix * lightPosition;\n"
                "    lightIntensity = dot (light, normal);\n"
                "\n"
                "    gl_Position = projectionMatrix * viewMatrix * position;\n"
                "}\n",

                SHADER_DEMO_HEADER
               #if JUCE_OPENGL_ES
                "varying highp float lightIntensity;\n"
               #else
                "varying float lightIntensity;\n"
               #endif
                "\n"
                "void main()\n"
                "{\n"
               #if JUCE_OPENGL_ES
                "   highp float l = lightIntensity * 0.25;\n"
                "   highp vec4 colour = vec4 (l, l, l, 1.0);\n"
               #else
                "   float l = lightIntensity * 0.25;\n"
                "   vec4 colour = vec4 (l, l, l, 1.0);\n"
               #endif
                "\n"
                "    gl_FragColor = colour;\n"
                "}\n"
            },

            {
                "Flattened",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec4 normal;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "uniform vec4 lightPosition;\n"
                "\n"
                "varying float lightIntensity;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    vec4 light = viewMatrix * lightPosition;\n"
                "    lightIntensity = dot (light, normal);\n"
                "\n"
                "    vec4 v = vec4 (position);\n"
                "    v.z = v.z * 0.1;\n"
                "\n"
                "    gl_Position = projectionMatrix * viewMatrix * v;\n"
                "}\n",

                SHADER_DEMO_HEADER
               #if JUCE_OPENGL_ES
                "varying highp float lightIntensity;\n"
               #else
                "varying float lightIntensity;\n"
               #endif
                "\n"
                "void main()\n"
                "{\n"
               #if JUCE_OPENGL_ES
                "   highp float l = lightIntensity * 0.25;\n"
                "   highp vec4 colour = vec4 (l, l, l, 1.0);\n"
               #else
                "   float l = lightIntensity * 0.25;\n"
                "   vec4 colour = vec4 (l, l, l, 1.0);\n"
               #endif
                "\n"
                "    gl_FragColor = colour;\n"
                "}\n"
            },

            {
                "Toon Shader",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec4 normal;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "uniform vec4 lightPosition;\n"
                "\n"
                "varying float lightIntensity;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    vec4 light = viewMatrix * lightPosition;\n"
                "    lightIntensity = dot (light, normal);\n"
                "\n"
                "    gl_Position = projectionMatrix * viewMatrix * position;\n"
                "}\n",

                SHADER_DEMO_HEADER
               #if JUCE_OPENGL_ES
                "varying highp float lightIntensity;\n"
               #else
                "varying float lightIntensity;\n"
               #endif
                "\n"
                "void main()\n"
                "{\n"
               #if JUCE_OPENGL_ES
                "    highp float intensity = lightIntensity * 0.5;\n"
                "    highp vec4 colour;\n"
               #else
                "    float intensity = lightIntensity * 0.5;\n"
                "    vec4 colour;\n"
               #endif
                "\n"
                "    if (intensity > 0.95)\n"
                "        colour = vec4 (1.0, 0.5, 0.5, 1.0);\n"
                "    else if (intensity > 0.5)\n"
                "        colour  = vec4 (0.6, 0.3, 0.3, 1.0);\n"
                "    else if (intensity > 0.25)\n"
                "        colour  = vec4 (0.4, 0.2, 0.2, 1.0);\n"
                "    else\n"
                "        colour  = vec4 (0.2, 0.1, 0.1, 1.0);\n"
                "\n"
                "    gl_FragColor = colour;\n"
                "}\n"
            },

            {
                "Orange Shader",

                SHADER_DEMO_HEADER
                "attribute vec4 position;\n"
                "attribute vec4 sourceColour;\n"
                "attribute vec2 textureCoordIn;\n"
                "\n"
                "uniform mat4 projectionMatrix;\n"
                "uniform mat4 viewMatrix;\n"
                "uniform mat4 microphonePosition;\n"
                "uniform mat4 speakerPosition;\n"
                "\n"
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
                "\n"
                "void main()\n"
                "{\n"
                "    destinationColour = sourceColour;\n"
                "    textureCoordOut = textureCoordIn;\n"
                "    gl_Position = projectionMatrix * viewMatrix * position;\n"
                "}\n",

                SHADER_DEMO_HEADER
               #if JUCE_OPENGL_ES
                "varying lowp vec4 destinationColour;\n"
                "varying lowp vec2 textureCoordOut;\n"
               #else
                "varying vec4 destinationColour;\n"
                "varying vec2 textureCoordOut;\n"
               #endif
                "\n"
                "void main()\n"
                "{\n"
               #if JUCE_OPENGL_ES
                "   lowp vec4 colour = vec4(0.95, 0.57, 0.03, 0.7);\n"
               #else
                "   vec4 colour = vec4(0.95, 0.57, 0.03, 0.7);\n"
               #endif
                "   gl_FragColor = colour;\n"
                "}\n"
            }
        };

        return Array<ShaderPreset>(presets, numElementsInArray(presets));
    }
};

class OpenGLComponent: public juce::Component,
                       public juce::OpenGLRenderer,
                       public ChangeListener,
                       private juce::AsyncUpdater
{
public:
    OpenGLComponent(RaumsimulationAudioProcessor&, juce::AudioProcessorValueTreeState&, Raytracer&);
    ~OpenGLComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // OpenGLRenderer overrides

    void newOpenGLContextCreated() override;
    void openGLContextClosing() override;
    void renderOpenGL() override;

    void changeListenerCallback(juce::ChangeBroadcaster *source) override
    {
        controlsOverlay->repaint();
    }

    Matrix3D<float> getProjectionMatrix() const;
    Matrix3D<float> getViewMatrix() const;
    Matrix3D<float> getCoordProjectionMatrix() const;

    void setShaderProgram();

    Rectangle<int> bounds;
    Draggable3DOrientation draggableOrientation;
    SmoothedValue<float, ValueSmoothingTypes::Linear> scale = 0.5f, rotationSpeed = 0.0f;
    CriticalSection mutex;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGLComponent)

    void handleAsyncUpdate() override
    {
        const ScopedLock lock(shaderMutex); // Prevent concurrent access to shader strings and status
        controlsOverlay->statusLabel.setText(statusText, dontSendNotification);
    }

    juce::OpenGLContext openGLContext;

    RaumsimulationAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& parameters;
    Raytracer& raytracer;

    class ControlsOverlay : public Component,
                            private Timer
    {
    public:
        ControlsOverlay(OpenGLComponent& oGLC)
            : openGLComponent(oGLC)
        {
            addAndMakeVisible(statusLabel);
            statusLabel.setJustificationType(Justification::topLeft);

            addAndMakeVisible(zoomSlider);
            zoomSlider.setSliderStyle(juce::Slider::LinearBar);
            zoomSlider.setRange(0.0, 1.0, 0.001);
            zoomSlider.onValueChange = [this] { sliderChanged();
                                                openGLComponent.parameters.state.setProperty("zoom", zoomSlider.getValue(), nullptr); };
            float zoom = (float) openGLComponent.parameters.state.getProperty("zoom");
            zoomSlider.setValue(zoom, dontSendNotification);

            addAndMakeVisible(zoomLabel);
            zoomLabel.attachToComponent(&zoomSlider, true);

            addAndMakeVisible(speedSlider);
            speedSlider.setSliderStyle(juce::Slider::LinearBar);
            speedSlider.setRange(0.0, 0.5, 0.001);
            speedSlider.onValueChange = [this] { sliderChanged();
                                                 openGLComponent.parameters.state.setProperty("rotation_speed", speedSlider.getValue(), nullptr); };
            float rotation_speed = (float) openGLComponent.parameters.state.getProperty("rotation_speed");
            speedSlider.setValue(rotation_speed, dontSendNotification);
            speedSlider.setSkewFactor(0.5f);

            addAndMakeVisible(speedLabel);
            speedLabel.attachToComponent(&speedSlider, true);

            addAndMakeVisible(presetBox);
            presetBox.onChange = [this] { selectPreset(presetBox.getSelectedItemIndex()); };

            auto presets = OpenGLUtils::getPresets();

            for (int i = 0; i < presets.size(); ++i)
                presetBox.addItem(presets[i].name, i + 1);

            presetBox.setSelectedItemIndex(0);

            addAndMakeVisible(presetLabel);
            presetLabel.attachToComponent(&presetBox, true);

            addAndMakeVisible(objFileLoadButton);
            objFileLoadButton.onClick = [this] { openFile(); };

            objFileLabel.attachToComponent(&objFileLoadButton, false);
            objFileLabel.setText(openGLComponent.objFileURL.toString(false), sendNotificationAsync);

            lookAndFeelChanged();
        }

        void paint(juce::Graphics& g) override
        {
            auto area = getLocalBounds().reduced(5);

            if (openGLComponent.raytracer.minOrder < openGLComponent.raytracer.maxOrder) {   // Right side legend
                auto right = area.removeFromRight(10);
                right.reduce(0, area.getHeight()/4);

                float startHue  = 1.00f;
                float endHue    = 0.75f;

                auto gradient = ColourGradient::vertical(
                        Colour::fromHSV(startHue, 1.0f, 0.5f, 1.0f),
                        Colour::fromHSV(endHue, 1.0f, 0.5f, 1.0f),
                        right);

                float step = (1 - endHue) / (float) openGLComponent.raytracer.maxOrder;
                for (int i = openGLComponent.raytracer.minOrder; i < openGLComponent.raytracer.maxOrder; i++) {
                    gradient.addColour((float) i / (float) openGLComponent.raytracer.maxOrder, Colour::fromHSV(1.0f - step * (float) i, 1.0f, 0.5f, 1.0f));
                }

                g.setFillType(gradient);
                g.fillRoundedRectangle((float) right.getX(), (float) right.getY(), (float) right.getWidth(), (float) right.getHeight(), 5.0f);

                auto order = area.removeFromRight(100);
                order.reduce(0, area.getHeight()/4);

                auto min = order.removeFromTop(15);
                auto max = order.removeFromBottom(15);

                g.setFillType(getLookAndFeel().findColour(Label::textColourId));
                g.drawText(String(openGLComponent.raytracer.minOrder), min, Justification::right, false);
                g.drawText(String(openGLComponent.raytracer.maxOrder), max, Justification::right, false);

                // make the area a square so the text is vertical on the right side after a 90-degree rotation with top-justification
                order.expand(0, (order.getWidth() - order.getHeight()) / 2);
                g.addTransform(AffineTransform::rotation(MathConstants<float>::pi / 2.0f, (float) order.getCentreX(), (float) order.getCentreY()));
                g.drawText("Reflection Order", order, Justification::centredTop, false);
            }
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced(5);

            {   // Top
                auto top = area.removeFromTop(50);

                auto sliders = top.removeFromRight(1*area.getWidth()/5);
                speedSlider.setBounds(sliders.removeFromBottom(25));
                zoomSlider.setBounds(sliders.removeFromBottom(25));

                statusLabel.setBounds(top);
            }

            {   // Bottom
                auto bottom = area.removeFromBottom(25);
                objFileLoadButton.setBounds(bottom.removeFromRight(1*area.getWidth()/2));
            }
        }

        bool isMouseButtonDownThreadsafe() const { return buttonDown; }

        void mouseDown(const MouseEvent& e) override
        {
            const ScopedLock lock(openGLComponent.mutex);
            openGLComponent.draggableOrientation.mouseDown(e.getPosition());

            buttonDown = true;
        }

        void mouseDrag(const MouseEvent& e) override
        {
            const ScopedLock lock(openGLComponent.mutex);
            openGLComponent.draggableOrientation.mouseDrag(e.getPosition());
        }

        void mouseUp(const MouseEvent&) override
        {
            buttonDown = false;
        }

        void mouseWheelMove(const MouseEvent&, const MouseWheelDetails& d) override
        {
            zoomSlider.setValue(zoomSlider.getValue() + d.deltaY);
        }

        void mouseMagnify(const MouseEvent&, float magnifyAmount) override
        {
            zoomSlider.setValue(zoomSlider.getValue() + magnifyAmount - 1.0f);
        }

        void selectPreset(int preset)
        {
            const auto& p = OpenGLUtils::getPresets()[preset];

            startTimer(1);
        }

        void updateShader()
        {
            startTimer(10);
        }

        Label statusLabel;

    private:
        void sliderChanged()
        {
            const ScopedLock lock(openGLComponent.mutex);

            openGLComponent.scale = (float) zoomSlider.getValue();
            openGLComponent.rotationSpeed = (float) speedSlider.getValue();
        }

        enum { shaderLinkDelay = 500 };

        void timerCallback() override
        {
            stopTimer();
            openGLComponent.setShaderProgram();
        }

        OpenGLComponent& openGLComponent;

        Label   speedLabel{{}, "Speed"};
        Label   zoomLabel{{}, "Zoom"};

        ComboBox presetBox;

        Label presetLabel{ {}, "Shader Preset:" };

        Slider speedSlider, zoomSlider;

        std::atomic<bool> buttonDown{ false };

        void openFile()
        {
            if (objFileChooser != nullptr)
                return;

            objFileChooser.reset(new FileChooser ("Select an obj file...", File(), "*.obj"));

            objFileChooser->launchAsync(FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                    [this] (const FileChooser& fc) mutable
                    {
                        if (fc.getURLResults().size() > 0)
                        {
                            auto result = fc.getURLResult();
                            openGLComponent.objFileURL = result;
                            openGLComponent.parameters.state.setProperty("obj_file_url", result.toString(false), nullptr);
                            objFileLabel.setText(result.toString(false), sendNotificationAsync);

                            startTimer(10);
                            openGLComponent.audioProcessor.updateParameters();
                        }

                        objFileChooser = nullptr;
                    }, nullptr);
        }

        Label objFileLabel = {{}, "No file selected"};
        TextButton objFileLoadButton = {"Load OBJ File..." , "Choose a file that contains the 3D model for the room you want to simulate"};
        std::unique_ptr<juce::FileChooser> objFileChooser;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlsOverlay)
    };

    std::unique_ptr<ControlsOverlay> controlsOverlay;

    float rotation = 0.0f;

    juce::URL objFileURL = {};
    std::unique_ptr<OpenGLShaderProgram>        roomShader;
    std::unique_ptr<OpenGLUtils::Shape>         roomShape;
    std::shared_ptr<OpenGLUtils::Attributes>    roomAttributes;
    std::unique_ptr<OpenGLShaderProgram>        microphoneShader;
    std::unique_ptr<OpenGLUtils::Shape>         microphoneShape;
    std::shared_ptr<OpenGLUtils::Attributes>    microphoneAttributes;
    std::unique_ptr<OpenGLShaderProgram>        speakerShader;
    std::unique_ptr<OpenGLUtils::Shape>         speakerShape;
    std::shared_ptr<OpenGLUtils::Attributes>    speakerAttributes;
    std::unique_ptr<OpenGLShaderProgram>        visualizationShader;
    std::shared_ptr<OpenGLUtils::Attributes>    visualizationAttributes;
    std::unique_ptr<OpenGLShaderProgram>        coordShader;
    std::shared_ptr<OpenGLUtils::Attributes>    coordAttributes;
    std::unique_ptr<OpenGLShaderProgram>        floodShader;
    std::shared_ptr<OpenGLUtils::Attributes>    floodAttributes;

    CriticalSection shaderMutex;
    String statusText;
    String newVisualizationVertexShader, newVisualizationFragmentShader;
    String newCoordVertexShader, newCoordFragmentShader;
    String newFloodVertexShader, newFloodFragmentShader;

    bool shadersShouldUpdate = false;

    std::unique_ptr<Shader> genericShader = nullptr;
    std::unique_ptr<Shader> roomRRRShader = nullptr;
    std::unique_ptr<Shader> microphoneRRRShader = nullptr;
    std::unique_ptr<Shader> speakerRRRShader = nullptr;

    void updateShader()
    {
        const ScopedLock lock(shaderMutex); // Prevent concurrent access to shader strings and status

        if (!shadersShouldUpdate) return;

        // ROOM
        {
            std::unique_ptr<OpenGLShaderProgram> newRoomShader(new OpenGLShaderProgram(openGLContext));

            if (newRoomShader->addVertexShader(newVisualizationVertexShader)
                && newRoomShader->addFragmentShader(newVisualizationFragmentShader)
                && newRoomShader->link())
            {
                roomShape.reset();
                roomAttributes.reset();

                roomShader.reset(newRoomShader.release());
                roomShader->use();


                if (objFileURL.isEmpty())
                {
                    roomShape.reset(new OpenGLUtils::Shape());
                }
                else
                {
                    roomShape.reset(new OpenGLUtils::Shape(objFileURL.getLocalFile()));
                }

                roomAttributes.reset(new OpenGLUtils::Attributes(*roomRRRShader->getShaderProgram()));

                statusText = "GLSL: v" + String(OpenGLShaderProgram::getLanguageVersion(), 2);
            }
        }

        // VISUALIZATION
        {
            std::unique_ptr<OpenGLShaderProgram> newVisualizationShader(new OpenGLShaderProgram(openGLContext));

            if (newVisualizationShader->addVertexShader(newVisualizationVertexShader)
                && newVisualizationShader->addFragmentShader(newVisualizationFragmentShader)
                && newVisualizationShader->link())
            {
                visualizationAttributes.reset();

                visualizationShader.reset(newVisualizationShader.release());
                visualizationShader->use();

                visualizationAttributes.reset(new OpenGLUtils::Attributes(*visualizationShader));

                statusText = "GLSL: v" + String(OpenGLShaderProgram::getLanguageVersion(), 2);
            } else {
                statusText = newVisualizationShader->getLastError();
            }
        }

        // COORDINATE AXIS
        {
            std::unique_ptr<OpenGLShaderProgram> newCoordShader(new OpenGLShaderProgram(openGLContext));

            if (newCoordShader->addVertexShader(newCoordVertexShader)
                && newCoordShader->addFragmentShader(newCoordFragmentShader)
                && newCoordShader->link())
            {
                coordAttributes.reset();

                coordShader.reset(newCoordShader.release());
                coordShader->use();

                coordAttributes.reset(new OpenGLUtils::Attributes(*coordShader));

                statusText = "GLSL: v" + String(OpenGLShaderProgram::getLanguageVersion(), 2);
            } else {
                statusText = newCoordShader->getLastError();
            }
        }

        // FLOOD FILL
        {
            std::unique_ptr<OpenGLShaderProgram> newFloodShader(new OpenGLShaderProgram(openGLContext));

            if (newFloodShader->addVertexShader(newFloodVertexShader)
                && newFloodShader->addFragmentShader(newFloodFragmentShader)
                && newFloodShader->link())
            {
                floodAttributes.reset();

                floodShader.reset(newFloodShader.release());
                floodShader->use();

                floodAttributes.reset(new OpenGLUtils::Attributes(*floodShader));

                statusText = "GLSL: v" + String(OpenGLShaderProgram::getLanguageVersion(), 2);
            } else {
                statusText = newFloodShader->getLastError();
            }
        }

        // MICROPHONES
        {
            std::unique_ptr<OpenGLShaderProgram> newMicrophoneShader(new OpenGLShaderProgram(openGLContext));

            if (true)
            {
                microphoneShape.reset();
                microphoneAttributes.reset();

                microphoneShader.reset(newMicrophoneShader.release());
                microphoneShader->use();

                auto headFileStream = std::make_unique<MemoryInputStream>(BinaryData::head_obj, BinaryData::head_objSize, true);
                microphoneShape.reset(new OpenGLUtils::Shape(String(CharPointer_UTF8((const char*) headFileStream->getData()))));
                microphoneAttributes.reset(new OpenGLUtils::Attributes(*microphoneRRRShader->getShaderProgram()));

                statusText = "GLSL: v" + String(OpenGLShaderProgram::getLanguageVersion(), 2);
            }
        }

        // SPEAKERS
        {
            std::unique_ptr<OpenGLShaderProgram> newSpeakerShader(new OpenGLShaderProgram(openGLContext));

            if (true)
            {
                speakerShape.reset();
                speakerAttributes.reset();

                speakerShader.reset(newSpeakerShader.release());
                speakerShader->use();

                auto ballFileStream = std::make_unique<MemoryInputStream>(BinaryData::ball_obj, BinaryData::ball_objSize, true);
                speakerShape.reset(new OpenGLUtils::Shape(String(CharPointer_UTF8((const char*) ballFileStream->getData()))));
                speakerAttributes.reset(new OpenGLUtils::Attributes(*speakerRRRShader->getShaderProgram()));

                statusText = "GLSL: v" + String(OpenGLShaderProgram::getLanguageVersion(), 2);
            }
        }

        triggerAsyncUpdate();

        shadersShouldUpdate = false;
    }
};
