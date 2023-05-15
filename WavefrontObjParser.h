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

#include <map>

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

    struct Vertex        { float x, y, z; };
    struct TextureCoord  { float x, y;    };

    struct Mesh
    {
        Array<Vertex> vertices, normals;
        Array<TextureCoord> textureCoords;
        Array<Index> indices;
    };

    struct Material
    {
        Material() noexcept
        {
            zerostruct (ambient);
            zerostruct (diffuse);
            zerostruct (specular);
            zerostruct (transmittance);
            zerostruct (emission);
        }

        String name;

        Vertex ambient, diffuse, specular, transmittance, emission;
        float shininess = 1.0f, refractiveIndex = 0.0f;

        String ambientTextureName, diffuseTextureName,
               specularTextureName, normalTextureName;

        StringPairArray parameters;
    };

    struct Surface {
        Array<Vertex> vertices;
        Vertex normal;

    };

    struct AbsorptionCoefficients {
        float* absorption_coefficients = new float[6]{1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,};

        float& operator[](int index) const {
            if (0 <= index && index < 6) {
                return absorption_coefficients[index];
            } else {
                throw std::out_of_range("You tried to access an out of range absorption coefficient. There are only 6 coefficients.");
            }
        }
    };

    struct Shape
    {
        String name;
        Mesh mesh;
        Array<Surface> surfaces;
        AbsorptionCoefficients absorption_coefficients;
        Material material;
    };

    OwnedArray<Shape> shapes;

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

            if (isPositiveAndBelow (i.vertexIndex, srcMesh.vertices.size()))
                newMesh.vertices.add (srcMesh.vertices.getReference (i.vertexIndex));

            if (isPositiveAndBelow (i.normalIndex, srcMesh.normals.size()))
                newMesh.normals.add (srcMesh.normals.getReference (i.normalIndex));

            if (isPositiveAndBelow (i.textureIndex, srcMesh.textureCoords.size()))
                newMesh.textureCoords.add (srcMesh.textureCoords.getReference (i.textureIndex));

            map[i] = index;
            return index;
        }
    };

    static float parseFloat (String::CharPointerType& t)
    {
        t.incrementToEndOfWhitespace();
        return (float) CharacterFunctions::readDoubleValue (t);
    }

    static Vertex parseVertex (String::CharPointerType t)
    {
        Vertex v;
        v.x = parseFloat (t);
        v.y = parseFloat (t);
        v.z = parseFloat (t);
        return v;
    }

    static TextureCoord parseTextureCoord (String::CharPointerType t)
    {
        TextureCoord tc;
        tc.x = parseFloat (t);
        tc.y = parseFloat (t);
        return tc;
    }

    static AbsorptionCoefficients parseAbsorptionCoefficients(String::CharPointerType t)
    {
        AbsorptionCoefficients absorptionCoefficients;

        if (*t != '\0') {           // Material is not empty
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
        }

        return absorptionCoefficients;
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

    struct Face
    {
        Face (String::CharPointerType t)
        {
            while (! t.isEmpty())
                triples.add (parseTriple (t));
        }

        Array<TripleIndex> triples;

        void addIndices (Mesh& newMesh, const Mesh& srcMesh, IndexMap& indexMap)
        {
            TripleIndex i0 (triples[0]), i1, i2 (triples[1]);

            for (auto i = 2; i < triples.size(); ++i)
            {
                i1 = i2;
                i2 = triples.getReference (i);

                newMesh.indices.add (indexMap.getIndexFor (i0, newMesh, srcMesh));
                newMesh.indices.add (indexMap.getIndexFor (i1, newMesh, srcMesh));
                newMesh.indices.add (indexMap.getIndexFor (i2, newMesh, srcMesh));
            }
        }

        static TripleIndex parseTriple (String::CharPointerType& t)
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
            t = findEndOfFaceToken (t);
            return i;
        }

        static String::CharPointerType findEndOfFaceToken (String::CharPointerType t) noexcept
        {
            return CharacterFunctions::findEndOfToken (t, CharPointer_ASCII ("/ \t"), String().getCharPointer());
        }
    };

    static Shape* parseFaceGroup (const Mesh& srcMesh,
                                  Array<Face>& faceGroup,
                                  const Material& material,
                                  const String& name)
    {
        if (faceGroup.size() == 0)
            return nullptr;

        std::unique_ptr<Shape> shape (new Shape());
        shape->name = name;
        shape->material = material;
        shape->absorption_coefficients = parseAbsorptionCoefficients(material.name.getCharPointer());

        IndexMap indexMap;

        for (auto& f : faceGroup) {
            f.addIndices (shape->mesh, srcMesh, indexMap);

            Surface surface;

            for (TripleIndex index : f.triples) {
                surface.normal = srcMesh.normals[index.normalIndex];
                surface.vertices.add(srcMesh.vertices[index.vertexIndex]);
            }

            shape->surfaces.add(surface);
        }

        return shape.release();
    }

    Result parseObjFile (const StringArray& lines)
    {
        Mesh mesh;
        Array<Face> faceGroup;

        Array<Material> knownMaterials;
        Material lastMaterial;
        String lastName;

        for (auto lineNum = 0; lineNum < lines.size(); ++lineNum)
        {
            auto l = lines[lineNum].getCharPointer().findEndOfWhitespace();

            if (matchToken (l, "v"))    { mesh.vertices     .add (parseVertex (l));       continue; }
            if (matchToken (l, "vn"))   { mesh.normals      .add (parseVertex (l));       continue; }
            if (matchToken (l, "vt"))   { mesh.textureCoords.add (parseTextureCoord (l)); continue; }
            if (matchToken (l, "f"))    { faceGroup         .add (Face (l));              continue; }

            if (matchToken (l, "usemtl"))
            {
                auto name = String (l).trim();

                for (auto i = knownMaterials.size(); --i >= 0;)
                {
                    if (knownMaterials.getReference (i).name == name)
                    {
                        lastMaterial = knownMaterials.getReference (i);
                        break;
                    }
                }

                continue;
            }

            if (matchToken (l, "mtllib"))
            {
                auto r = parseMaterial (knownMaterials, String (l).trim());
                continue;
            }

            if (matchToken (l, "g") || matchToken (l, "o"))
            {
                if (auto* shape = parseFaceGroup (mesh, faceGroup, lastMaterial, lastName))
                    shapes.add (shape);

                faceGroup.clear();
                lastName = StringArray::fromTokens (l, " \t", "")[0];
                continue;
            }
        }

        if (auto* shape = parseFaceGroup (mesh, faceGroup, lastMaterial, lastName))
            shapes.add (shape);

        return Result::ok();
    }

    Result parseMaterial (Array<Material>& materials, const String& filename)
    {
        jassert (sourceFile.exists());
        auto f = sourceFile.getSiblingFile (filename);

        if (! f.exists())
            return Result::fail ("Cannot open file: " + filename);

        auto lines = StringArray::fromLines (f.loadFileAsString());

        materials.clear();
        Material material;

        for (auto line : lines)
        {
            auto l = line.getCharPointer().findEndOfWhitespace();

            if (matchToken (l, "newmtl"))   { materials.add (material); material.name = String (l).trim(); continue; }

            if (matchToken (l, "Ka"))       { material.ambient         = parseVertex (l); continue; }
            if (matchToken (l, "Kd"))       { material.diffuse         = parseVertex (l); continue; }
            if (matchToken (l, "Ks"))       { material.specular        = parseVertex (l); continue; }
            if (matchToken (l, "Kt"))       { material.transmittance   = parseVertex (l); continue; }
            if (matchToken (l, "Ke"))       { material.emission        = parseVertex (l); continue; }
            if (matchToken (l, "Ni"))       { material.refractiveIndex = parseFloat (l);  continue; }
            if (matchToken (l, "Ns"))       { material.shininess       = parseFloat (l);  continue; }

            if (matchToken (l, "map_Ka"))   { material.ambientTextureName  = String (l).trim(); continue; }
            if (matchToken (l, "map_Kd"))   { material.diffuseTextureName  = String (l).trim(); continue; }
            if (matchToken (l, "map_Ks"))   { material.specularTextureName = String (l).trim(); continue; }
            if (matchToken (l, "map_Ns"))   { material.normalTextureName   = String (l).trim(); continue; }

            auto tokens = StringArray::fromTokens (l, " \t", "");

            if (tokens.size() >= 2)
                material.parameters.set (tokens[0].trim(), tokens[1].trim());
        }

        materials.add (material);
        return Result::ok();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavefrontObjFile)
};
