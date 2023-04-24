#pragma once

#include <JuceHeader.h>
#include "WavefrontObjParser.h"

struct OpenGLUtils
{
    //==============================================================================
    /** Vertex data to be passed to the shaders.
        For the purposes of this demo, each vertex will have a 3D position, a colour and a
        2D texture co-ordinate. Of course you can ignore these or manipulate them in the
        shader programs but are some useful defaults to work from.
     */
    struct Vertex
    {
        float position[3];
        float normal[3];
        float colour[4];
    };

    //==============================================================================
    // This class just manages the attributes that the demo shaders use.
    struct Attributes
    {
        explicit Attributes(OpenGLShaderProgram& shader)
        {
            position.reset(createAttribute(shader, "position"));
            normal.reset(createAttribute(shader, "normal"));
            sourceColour.reset(createAttribute(shader, "sourceColour"));
        }

        void enable()
        {
            using namespace ::juce::gl;

            if (position.get() != nullptr)
            {
                glVertexAttribPointer(position->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
                glEnableVertexAttribArray(position->attributeID);
            }

            if (normal.get() != nullptr)
            {
                glVertexAttribPointer(normal->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(sizeof(float) * 3));
                glEnableVertexAttribArray(normal->attributeID);
            }

            if (sourceColour.get() != nullptr)
            {
                glVertexAttribPointer(sourceColour->attributeID, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)(sizeof(float) * 6));
                glEnableVertexAttribArray(sourceColour->attributeID);
            }
        }

        void disable()
        {
            using namespace ::juce::gl;

            if (position.get() != nullptr)        glDisableVertexAttribArray(position->attributeID);
            if (normal.get() != nullptr)          glDisableVertexAttribArray(normal->attributeID);
            if (sourceColour.get() != nullptr)    glDisableVertexAttribArray(sourceColour->attributeID);
        }

        std::unique_ptr<OpenGLShaderProgram::Attribute> position, normal, sourceColour;

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

    //==============================================================================
    // This class just manages the uniform values that the demo shaders use.
    struct Uniforms
    {
        explicit Uniforms(OpenGLShaderProgram& shader)
        {
            projectionMatrix.reset(createUniform(shader, "projectionMatrix"));
            viewMatrix.reset(createUniform(shader, "viewMatrix"));
            lightPosition.reset(createUniform(shader, "lightPosition"));
            bouncingNumber.reset(createUniform(shader, "bouncingNumber"));
        }

        std::unique_ptr<OpenGLShaderProgram::Uniform> projectionMatrix, viewMatrix, lightPosition, bouncingNumber;

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

    //==============================================================================
    /** This loads a 3D model from an OBJ file and converts it into some vertex buffers
        that we can draw.
    */
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
                    shape.mesh.indices.getRawDataPointer(), GL_STATIC_DRAW);
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
            auto scale = 0.2f;
            WavefrontObjFile::TextureCoord defaultTexCoord = { 0.5f, 0.5f };
            WavefrontObjFile::Vertex defaultNormal = { 0.5f, 0.5f, 0.5f };

            for (int i = 0; i < mesh.vertices.size(); ++i)
            {
                auto& v = mesh.vertices.getReference(i);

                auto& n = (i < mesh.normals.size() ? mesh.normals.getReference(i)
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

//==============================================================================
/** This is the main component - the GL context gets attached to it, and
    it implements the OpenGLRenderer callback so that it can do real GL work.
*/
class OpenGLComponent: public juce::Component,
                       public juce::OpenGLRenderer,
                       private juce::AsyncUpdater
{
public:
    OpenGLComponent(juce::AudioProcessorValueTreeState&);
    ~OpenGLComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // OpenGLRenderer overrides

    void newOpenGLContextCreated() override;
    void openGLContextClosing() override;
    void renderOpenGL() override;

    Matrix3D<float> getProjectionMatrix() const;
    Matrix3D<float> getViewMatrix() const;
    void setShaderProgram(const String& vertexShader, const String& fragmentShader);

    Rectangle<int> bounds;
    Draggable3DOrientation draggableOrientation;
    bool doBackgroundDrawing = false;
    float scale = 0.5f, rotationSpeed = 0.0f;
    CriticalSection mutex;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenGLComponent)

    void handleAsyncUpdate() override
    {
        const ScopedLock lock(shaderMutex); // Prevent concurrent access to shader strings and status
        controlsOverlay->statusLabel.setText(statusText, dontSendNotification);
    }

    juce::OpenGLContext openGLContext;

    juce::AudioProcessorValueTreeState& parameters;

    //==============================================================================
    /**
        This component sits on top of the main GL demo, and contains all the sliders
        and widgets that control things.
    */
    class ControlsOverlay : public Component,
                            private CodeDocument::Listener,
                            private Slider::Listener,
                            private Timer
    {
    public:
        ControlsOverlay(OpenGLComponent& oGLC)
            : openGLComponent(oGLC)
        {
            addAndMakeVisible(statusLabel);
            statusLabel.setJustificationType(Justification::topLeft);
            statusLabel.setFont(Font(14.0f));

            addAndMakeVisible(sizeSlider);
            sizeSlider.setRange(0.0, 1.0, 0.001);
            sizeSlider.addListener(this);

            addAndMakeVisible(zoomLabel);
            zoomLabel.attachToComponent(&sizeSlider, true);

            addAndMakeVisible(speedSlider);
            speedSlider.setRange(0.0, 0.5, 0.001);
            speedSlider.addListener(this);
            speedSlider.setSkewFactor(0.5f);

            addAndMakeVisible(speedLabel);
            speedLabel.attachToComponent(&speedSlider, true);

            vertexDocument.addListener(this);
            fragmentDocument.addListener(this);

            addAndMakeVisible(presetBox);
            presetBox.onChange = [this] { selectPreset(presetBox.getSelectedItemIndex()); };

            auto presets = OpenGLUtils::getPresets();

            for (int i = 0; i < presets.size(); ++i)
                presetBox.addItem(presets[i].name, i + 1);

            addAndMakeVisible(presetLabel);
            presetLabel.attachToComponent(&presetBox, true);

            addAndMakeVisible(objFileLoadButton);
            objFileLoadButton.setColour(TextButton::buttonColourId, Colour (0xff797fed));
            objFileLoadButton.setColour(TextButton::textColourOffId, Colours::black);
            objFileLoadButton.onClick = [this] { openFile(); };

            objFileLabel.attachToComponent(&objFileLoadButton, false);
            objFileLabel.setText(openGLComponent.objFileURL.toString(false), sendNotificationAsync);

            lookAndFeelChanged();
        }

        void initialise()
        {
            presetBox.setSelectedItemIndex(0);
            speedSlider.setValue(0.01);
            sizeSlider.setValue(0.5);
        }

        void resized() override
        {
            auto area = getLocalBounds().reduced(5);

            auto top = area.removeFromTop(75);

            auto sliders = top.removeFromRight(area.getWidth() / 2);
            speedSlider.setBounds(sliders.removeFromBottom(25));
            sizeSlider.setBounds(sliders.removeFromBottom(25));

            top.removeFromRight(70);
            statusLabel.setBounds(top);

            auto bottom = area.removeFromBottom(25);
            presetBox.setBounds(bottom.removeFromLeft(bottom.getWidth() / 3));
            bottom.removeFromLeft(5);       // little space between combo box and file chooser button
            objFileLoadButton.setBounds(bottom.removeFromRight(bottom.getWidth()));
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
            sizeSlider.setValue(sizeSlider.getValue() + d.deltaY);
        }

        void mouseMagnify(const MouseEvent&, float magnifyAmmount) override
        {
            sizeSlider.setValue(sizeSlider.getValue() + magnifyAmmount - 1.0f);
        }

        void selectPreset(int preset)
        {
            const auto& p = OpenGLUtils::getPresets()[preset];

            vertexDocument.replaceAllContent(p.vertexShader);
            fragmentDocument.replaceAllContent(p.fragmentShader);

            startTimer(1);
        }

        void updateShader()
        {
            startTimer(10);
        }

        Label statusLabel;

    private:
        void sliderValueChanged(Slider*) override
        {
            const ScopedLock lock(openGLComponent.mutex);

            openGLComponent.scale = (float)sizeSlider.getValue();
            openGLComponent.rotationSpeed = (float)speedSlider.getValue();
        }

        enum { shaderLinkDelay = 500 };

        void codeDocumentTextInserted(const String& /*newText*/, int /*insertIndex*/) override
        {
            startTimer(shaderLinkDelay);
        }

        void codeDocumentTextDeleted(int /*startIndex*/, int /*endIndex*/) override
        {
            startTimer(shaderLinkDelay);
        }

        void timerCallback() override
        {
            stopTimer();
            openGLComponent.setShaderProgram(vertexDocument.getAllContent(),
                fragmentDocument.getAllContent());
        }

        OpenGLComponent& openGLComponent;

        Label speedLabel{ {}, "Speed:" },
            zoomLabel{ {}, "Zoom:" };

        CodeDocument vertexDocument, fragmentDocument;

        ComboBox presetBox;

        Label presetLabel{ {}, "Shader Preset:" };

        Slider speedSlider, sizeSlider;

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
    std::unique_ptr<OpenGLShaderProgram> shader;
    std::unique_ptr<OpenGLUtils::Shape> shape;
    std::unique_ptr<OpenGLUtils::Attributes> attributes;
    std::unique_ptr<OpenGLUtils::Uniforms> uniforms;

    CriticalSection shaderMutex;
    String newVertexShader, newFragmentShader, statusText;

    void updateShader()
    {
        const ScopedLock lock(shaderMutex); // Prevent concurrent access to shader strings and status

        if (newVertexShader.isNotEmpty() || newFragmentShader.isNotEmpty())
        {
            std::unique_ptr<OpenGLShaderProgram> newShader(new OpenGLShaderProgram(openGLContext));

            if (newShader->addVertexShader(OpenGLHelpers::translateVertexShaderToV3(newVertexShader))
                && newShader->addFragmentShader(OpenGLHelpers::translateFragmentShaderToV3(newFragmentShader))
                && newShader->link())
            {
                shape.reset();
                attributes.reset();
                uniforms.reset();

                shader.reset(newShader.release());
                shader->use();


                if (objFileURL.isEmpty())
                {
                    shape.reset(new OpenGLUtils::Shape());
                }
                else
                {
                    shape.reset(new OpenGLUtils::Shape(objFileURL.getLocalFile()));
                }

                attributes.reset(new OpenGLUtils::Attributes(*shader));
                uniforms.reset(new OpenGLUtils::Uniforms(*shader));

                statusText = "GLSL: v" + String(OpenGLShaderProgram::getLanguageVersion(), 2);
            }
            else
            {
                statusText = newShader->getLastError();
            }

            triggerAsyncUpdate();

            newVertexShader = {};
            newFragmentShader = {};
        }
    }
};
