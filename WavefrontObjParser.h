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

#pragma once

#include "CustomDatatypes.h"
#include "externals/glm/glm/glm.hpp"
#include "externals/glm/glm/ext.hpp"

using namespace juce;
//==============================================================================
/**
    This is a quick-and-dirty parser for the 3D OBJ file format.

    Just call load() and if there aren't any errors, the 'shapes' array should
    be filled with all the shape objects that were loaded from the file.
*/
class WavefrontObjFile
{
public:
    WavefrontObjFile() {}

    Result load (const String& objFileContent)
    {
        shapes.clear();
        return parseObjFile (StringArray::fromLines (objFileContent));
    }

    Result load (const File& file)
    {
        sourceFile = file;
        return load (file.loadFileAsString());
    }

    //==============================================================================
    typedef juce::uint32 Index;

    struct Mesh
    {
        std::vector<glm::vec3> vertices, normals;
        std::vector<Index> indices;
    };

    struct Material
    {
        String name;
    };

    struct Surface {
        std::vector<glm::vec3> vertices;
        glm::vec3 normal;
    };

    struct Shape
    {
        String name;
        Mesh mesh;
        std::vector<Surface> surfaces;
        MaterialProperties materialProperties;
        Material material;
    };

    std::vector<Shape*> shapes;

private:
    //==============================================================================
    File sourceFile;

    struct TripleIndex
    {
        TripleIndex() noexcept {}

        bool operator< (const TripleIndex& other) const noexcept
        {
            if (this == &other)
                return false;

            if (vertexIndex != other.vertexIndex)
                return vertexIndex < other.vertexIndex;

            if (textureIndex != other.textureIndex)
                return textureIndex < other.textureIndex;

            return normalIndex < other.normalIndex;
        }

        int vertexIndex = -1, textureIndex = -1, normalIndex = -1;
    };

    struct IndexMap
    {
        std::map<TripleIndex, Index> map;

        Index getIndexFor (TripleIndex i, Mesh& newMesh, const Mesh& srcMesh)
        {
            const std::map<TripleIndex, Index>::iterator it (map.find (i));

            if (it != map.end())
                return it->second;

            auto index = (Index) newMesh.vertices.size();

            if (isPositiveAndBelow(i.vertexIndex, srcMesh.vertices.size()))
                newMesh.vertices.push_back(srcMesh.vertices.at(i.vertexIndex));

            if (isPositiveAndBelow(i.normalIndex, srcMesh.normals.size()))
                newMesh.normals.push_back(srcMesh.normals.at(i.normalIndex));

            map[i] = index;
            return index;
        }
    };

    static float parseFloat(String::CharPointerType& t)
    {
        t.incrementToEndOfWhitespace();
        return (float) CharacterFunctions::readDoubleValue(t);
    }

    static glm::vec3 parseVector(String::CharPointerType t)
    {
        glm::vec3 v;
        v.x = parseFloat (t);
        v.y = parseFloat (t);
        v.z = parseFloat (t);
        return v;
    }

    static MaterialProperties parseMaterialProperties(String::CharPointerType t)
    {
        Band6Coefficients absorptionCoefficients;
        float roughness = 0.0f;

        if (*t != '\0') {           // Material is not empty

            // parse absorption coefficients
            while (*t != '[') {
                t++;
            }

            t++;

            for (int i = 0; i < 6; i++) {
                float value = (float) t.getDoubleValue();
                absorptionCoefficients[i] = value;

                while (*t != '/') {
                    if (*t == ']') {
                        break;
                    }
                    t++;
                }

                if (*t == '/') {
                    t++;
                }
            }

            // parse roughness value
            while (*t != '(') {
                t++;
            }

            t++;
            roughness = (float) t.getDoubleValue();
        }

        return MaterialProperties{absorptionCoefficients, roughness};
    }

    static bool matchToken(String::CharPointerType& t, const char* token)
    {
        auto len = (int) strlen(token);

        if (CharacterFunctions::compareUpTo(CharPointer_ASCII(token), t, len) == 0)
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

    struct Face
    {
        Face(String::CharPointerType t)
        {
            while(! t.isEmpty())
                triples.add(parseTriple (t));
        }

        Array<TripleIndex> triples;

        void addIndices(Mesh& newMesh, const Mesh& srcMesh, IndexMap& indexMap)
        {
            TripleIndex i0 (triples[0]), i1, i2 (triples[1]);

            for (auto i = 2; i < triples.size(); ++i)
            {
                i1 = i2;
                i2 = triples.getReference(i);

                newMesh.indices.push_back(indexMap.getIndexFor (i0, newMesh, srcMesh));
                newMesh.indices.push_back(indexMap.getIndexFor (i1, newMesh, srcMesh));
                newMesh.indices.push_back(indexMap.getIndexFor (i2, newMesh, srcMesh));
            }
        }

        static TripleIndex parseTriple(String::CharPointerType& t)
        {
            TripleIndex i;

            t.incrementToEndOfWhitespace();
            i.vertexIndex = t.getIntValue32() - 1;
            t = findEndOfFaceToken (t);

            if (t.isEmpty() || t.getAndAdvance() != '/')
                return i;

            if (*t == '/')
            {
                ++t;
            }
            else
            {
                i.textureIndex = t.getIntValue32() - 1;
                t = findEndOfFaceToken (t);

                if (t.isEmpty() || t.getAndAdvance() != '/')
                    return i;
            }

            i.normalIndex = t.getIntValue32() - 1;
            t = findEndOfFaceToken(t);
            return i;
        }

        static String::CharPointerType findEndOfFaceToken(String::CharPointerType t) noexcept
        {
            return CharacterFunctions::findEndOfToken(t, CharPointer_ASCII("/ \t"), String().getCharPointer());
        }
    };

    static Shape* parseFaceGroup(const Mesh& srcMesh,
                                 std::vector<Face>& faceGroup,
                                 const Material& material,
                                 const String& name)
    {
        if (faceGroup.empty())
            return nullptr;

        std::unique_ptr<Shape> shape (new Shape());
        shape->name = name;
        shape->material = material;
        shape->materialProperties = parseMaterialProperties(material.name.getCharPointer());

        IndexMap indexMap;

        for (auto& f : faceGroup) {
            f.addIndices(shape->mesh, srcMesh, indexMap);

            Surface surface;

            for (TripleIndex index : f.triples) {
                surface.normal = srcMesh.normals[index.normalIndex];
                surface.vertices.push_back(srcMesh.vertices[index.vertexIndex]);
            }

            shape->surfaces.push_back(surface);
        }

        return shape.release();
    }

    Result parseObjFile(const StringArray& lines)
    {
        Mesh mesh;
        std::vector<Face> faceGroup;

        std::vector<Material> knownMaterials;
        Material lastMaterial;
        String lastName;

        for (auto lineNum = 0; lineNum < lines.size(); ++lineNum)
        {
            auto l = lines[lineNum].getCharPointer().findEndOfWhitespace();

            if (matchToken (l, "v"))    { mesh.vertices     .push_back(parseVector(l));       continue; }
            if (matchToken (l, "vn"))   { mesh.normals      .push_back(parseVector(l));       continue; }
            if (matchToken (l, "f"))    { faceGroup         .emplace_back(l);                 continue; }   //constructs new element in-place

            if (matchToken (l, "usemtl"))
            {
                auto name = String(l).trim();

                lastMaterial = {name};

                continue;
            }

            if (matchToken (l, "g") || matchToken (l, "o"))
            {
                if (auto* shape = parseFaceGroup(mesh, faceGroup, lastMaterial, lastName))
                    shapes.push_back(shape);

                faceGroup.clear();
                lastName = StringArray::fromTokens(l, " \t", "")[0];
                continue;
            }
        }

        if (auto* shape = parseFaceGroup(mesh, faceGroup, lastMaterial, lastName))
            shapes.push_back(shape);

        return Result::ok();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavefrontObjFile)
};
