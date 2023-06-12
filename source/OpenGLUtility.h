#pragma once

#include "JuceHeader.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <utility>

class VertexBufferLayout
{
public:
    VertexBufferLayout()
    : stride(0)
    {
    }

    struct Element
    {
        unsigned int type;
        unsigned int count;
        bool normalized;

        static unsigned int sizeofType(unsigned int type)
        {
            switch (type) {
                case juce::gl::GL_FLOAT:        return 4;
                case juce::gl::GL_UNSIGNED_INT: return 4;
                case juce::gl::GL_BYTE:         return 1;
                default:
                    jassert(false);
                    return 0;
            }
        }
    };

    template<typename T>
    void addAttribute(unsigned int count, bool normalized)
    {
        jassert(false);
    }

    template<>
    void addAttribute<float>(unsigned int count, bool normalized)
    {
        elements.push_back({juce::gl::GL_FLOAT, count, normalized});
        stride += Element::sizeofType(juce::gl::GL_FLOAT);
    }

    inline unsigned int getStride() const
    {
        return stride;
    }

    inline const std::vector<Element>& getElements() const
    {
        return elements;
    }

private:
    std::vector<Element> elements;
    unsigned int stride;

};

class VertexBuffer
{
public:
    VertexBuffer(const void* data, unsigned int size)
    {
        using namespace juce::gl;

        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }

    ~VertexBuffer()
    {
        using namespace juce::gl;

        glDeleteBuffers(1, &vertexBuffer);
    }

    void addLayout(const VertexBufferLayout& vertexBufferLayout)
    {
        using namespace juce::gl;

        bind();

        const auto& attributeElements = vertexBufferLayout.getElements();
        unsigned int offset = 0;

        for (unsigned int elementNum = 0; elementNum < attributeElements.size(); elementNum++) {
            const auto& element = attributeElements[elementNum];

            glVertexAttribPointer(elementNum, element.count, element.type, element.normalized, vertexBufferLayout.getStride(), (GLvoid*) offset);
            glEnableVertexAttribArray(elementNum);

            offset += element.count * VertexBufferLayout::Element::sizeofType(element.type);
        }
    }

    void bind() const
    {
        using namespace juce::gl;

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    }

    void unbind() const
    {
        using namespace juce::gl;

        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

private:
    GLuint vertexBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VertexBuffer)
};

class IndexBuffer
{
public:
    IndexBuffer(const unsigned char* data, unsigned int count)
    : count(count)
    {
        jassert(sizeof (unsigned char) == sizeof (GLubyte));
        using namespace juce::gl;

        glGenBuffers(1, &indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(GLubyte), data, GL_STATIC_DRAW);
    }

    IndexBuffer(const unsigned short* data, unsigned int count)
    : count(count)
    {
        jassert(sizeof (unsigned short) == sizeof (GLushort));
        using namespace juce::gl;

        glGenBuffers(1, &indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(GLushort), data, GL_STATIC_DRAW);
    }

    IndexBuffer(const unsigned int* data, unsigned int count)
    : count(count)
    {
        jassert(sizeof (unsigned int) == sizeof (GLuint));
        using namespace juce::gl;

        glGenBuffers(1, &indexBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(GLuint), data, GL_STATIC_DRAW);
    }

    ~IndexBuffer()
    {
        using namespace juce::gl;

        glDeleteBuffers(1, &indexBuffer);
    }

    void bind() const
    {
        using namespace juce::gl;

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
    }

    void unbind() const
    {
        using namespace juce::gl;

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    inline unsigned int getCount() const
    {
        return count;
    }

private:
    GLuint indexBuffer;
    unsigned int count;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IndexBuffer)
};

class Shader
{
public:
    Shader(String vertexShader, String fragmentShader, OpenGLContext& context)
    : vertexShader(vertexShader)
    , fragmentShader(fragmentShader)
    , context(context)
    {
        compile();
    }

    ~Shader()
    {

    }

    void bind() const
    {
        juce::gl::glUseProgram(shaderProgram->getProgramID());
    }

    void unbind() const
    {
        juce::gl::glUseProgram(0);
    }

    void setUniformMatrix4(const String& name, float* data, unsigned int count, bool transpose)
    {
        juce::gl::glUniformMatrix4fv(getUniformLocation(name), count, transpose, data);
    }

    std::shared_ptr<OpenGLShaderProgram> getShaderProgram() const
    {
        return shaderProgram;
    }

private:
    void compile()
    {
        std::unique_ptr<OpenGLShaderProgram> newShader(new OpenGLShaderProgram(context));
        if (newShader->addShader(vertexShader, juce::gl::GL_VERTEX_SHADER) &&
            newShader->addShader(fragmentShader, juce::gl::GL_FRAGMENT_SHADER) &&
            newShader->link()) {
            shaderProgram = std::move(newShader);
        } else {
            String error = newShader->getLastError();
        }
    }

    int getUniformLocation(const String& name)
    {
        int location = juce::gl::glGetUniformLocation(shaderProgram->getProgramID(), name.getCharPointer());
        jassert(location != -1);

        return location;
    }

    String vertexShader;
    String fragmentShader;
    OpenGLContext& context;

    std::shared_ptr<OpenGLShaderProgram> shaderProgram = nullptr;
};
