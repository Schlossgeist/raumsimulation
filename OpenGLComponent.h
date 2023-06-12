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

    class ControlsOverlay : public Component
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

        Label statusLabel;

    private:
        void sliderChanged()
        {
            const ScopedLock lock(openGLComponent.mutex);

            openGLComponent.scale = (float) zoomSlider.getValue();
            openGLComponent.rotationSpeed = (float) speedSlider.getValue();
        }

        OpenGLComponent& openGLComponent;

        Label   speedLabel{{}, "Speed"};
        Label   zoomLabel{{}, "Zoom"};

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

                            openGLComponent.raytracer.clear();
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
    std::shared_ptr<OpenGLUtils::Shape>         roomShape;
    std::shared_ptr<OpenGLUtils::Attributes>    roomAttributes;
    std::shared_ptr<OpenGLUtils::Shape>         microphoneShape;
    std::shared_ptr<OpenGLUtils::Attributes>    microphoneAttributes;
    std::shared_ptr<OpenGLUtils::Shape>         speakerShape;
    std::shared_ptr<OpenGLUtils::Attributes>    speakerAttributes;
    std::shared_ptr<OpenGLUtils::Attributes>    visualizationAttributes;
    std::shared_ptr<OpenGLUtils::Attributes>    coordAttributes;
    std::shared_ptr<OpenGLUtils::Attributes>    floodAttributes;

    CriticalSection shaderMutex;
    String statusText;

    std::unique_ptr<Shader> genericShader = nullptr;
    std::unique_ptr<Shader> roomRRRShader = nullptr;
    std::unique_ptr<Shader> microphoneRRRShader = nullptr;
    std::unique_ptr<Shader> speakerRRRShader = nullptr;

    void updateRoomModel()
    {
        if (objFileURL.isEmpty()) {
            roomShape = std::make_shared<OpenGLUtils::Shape>();
        } else {
            roomShape = std::make_shared<OpenGLUtils::Shape>(objFileURL.getLocalFile());
        }
    }
};
