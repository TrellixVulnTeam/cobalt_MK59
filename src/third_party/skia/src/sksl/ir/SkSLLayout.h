/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SKSL_LAYOUT
#define SKSL_LAYOUT

#include "SkSLString.h"
#include "SkSLUtil.h"

namespace SkSL {

/**
 * Represents a layout block appearing before a variable declaration, as in:
 *
 * layout (location = 0) int x;
 */
struct Layout {
    enum Primitive {
        kUnspecified_Primitive = -1,
        kPoints_Primitive,
        kLines_Primitive,
        kLineStrip_Primitive,
        kLinesAdjacency_Primitive,
        kTriangles_Primitive,
        kTriangleStrip_Primitive,
        kTrianglesAdjacency_Primitive
    };

    // These are used by images in GLSL. We only support a subset of what GL supports.
    enum class Format {
        kUnspecified = -1,
        kRGBA32F,
        kR32F,
        kRGBA16F,
        kR16F,
        kRGBA8,
        kR8,
        kRGBA8I,
        kR8I,
    };

    // used by SkSL processors
    enum Key {
        // field is not a key
        kNo_Key,
        // field is a key
        kKey_Key,
        // key is 0 or 1 depending on whether the matrix is an identity matrix
        kIdentity_Key,
    };

    static const char* FormatToStr(Format format) {
        switch (format) {
            case Format::kUnspecified:  return "";
            case Format::kRGBA32F:      return "rgba32f";
            case Format::kR32F:         return "r32f";
            case Format::kRGBA16F:      return "rgba16f";
            case Format::kR16F:         return "r16f";
            case Format::kRGBA8:        return "rgba8";
            case Format::kR8:           return "r8";
            case Format::kRGBA8I:       return "rgba8i";
            case Format::kR8I:          return "r8i";
        }
        ABORT("Unexpected format");
    }

    static bool ReadFormat(String str, Format* format) {
        if (str == "rgba32f") {
            *format = Format::kRGBA32F;
            return true;
        } else if (str == "r32f") {
            *format = Format::kR32F;
            return true;
        } else if (str == "rgba16f") {
            *format = Format::kRGBA16F;
            return true;
        } else if (str == "r16f") {
            *format = Format::kR16F;
            return true;
        } else if (str == "rgba8") {
            *format = Format::kRGBA8;
            return true;
        } else if (str == "r8") {
            *format = Format::kR8;
            return true;
        } else if (str == "rgba8i") {
            *format = Format::kRGBA8I;
            return true;
        } else if (str == "r8i") {
            *format = Format::kR8I;
            return true;
        }
        return false;
    }

    Layout(int location, int offset, int binding, int index, int set, int builtin,
           int inputAttachmentIndex, bool originUpperLeft, bool overrideCoverage,
           bool blendSupportAllEquations, Format format, bool pushconstant, Primitive primitive,
           int maxVertices, int invocations, String when, Key key)
    : fLocation(location)
    , fOffset(offset)
    , fBinding(binding)
    , fIndex(index)
    , fSet(set)
    , fBuiltin(builtin)
    , fInputAttachmentIndex(inputAttachmentIndex)
    , fOriginUpperLeft(originUpperLeft)
    , fOverrideCoverage(overrideCoverage)
    , fBlendSupportAllEquations(blendSupportAllEquations)
    , fFormat(format)
    , fPushConstant(pushconstant)
    , fPrimitive(primitive)
    , fMaxVertices(maxVertices)
    , fInvocations(invocations)
    , fWhen(when)
    , fKey(key) {}

    Layout()
    : fLocation(-1)
    , fOffset(-1)
    , fBinding(-1)
    , fIndex(-1)
    , fSet(-1)
    , fBuiltin(-1)
    , fInputAttachmentIndex(-1)
    , fOriginUpperLeft(false)
    , fOverrideCoverage(false)
    , fBlendSupportAllEquations(false)
    , fFormat(Format::kUnspecified)
    , fPushConstant(false)
    , fPrimitive(kUnspecified_Primitive)
    , fMaxVertices(-1)
    , fInvocations(-1)
    , fKey(kNo_Key) {}

    String description() const {
        String result;
        String separator;
        if (fLocation >= 0) {
            result += separator + "location = " + to_string(fLocation);
            separator = ", ";
        }
        if (fOffset >= 0) {
            result += separator + "offset = " + to_string(fOffset);
            separator = ", ";
        }
        if (fBinding >= 0) {
            result += separator + "binding = " + to_string(fBinding);
            separator = ", ";
        }
        if (fIndex >= 0) {
            result += separator + "index = " + to_string(fIndex);
            separator = ", ";
        }
        if (fSet >= 0) {
            result += separator + "set = " + to_string(fSet);
            separator = ", ";
        }
        if (fBuiltin >= 0) {
            result += separator + "builtin = " + to_string(fBuiltin);
            separator = ", ";
        }
        if (fInputAttachmentIndex >= 0) {
            result += separator + "input_attachment_index = " + to_string(fInputAttachmentIndex);
            separator = ", ";
        }
        if (fOriginUpperLeft) {
            result += separator + "origin_upper_left";
            separator = ", ";
        }
        if (fOverrideCoverage) {
            result += separator + "override_coverage";
            separator = ", ";
        }
        if (fBlendSupportAllEquations) {
            result += separator + "blend_support_all_equations";
            separator = ", ";
        }
        if (Format::kUnspecified != fFormat) {
            result += separator + FormatToStr(fFormat);
            separator = ", ";
        }
        if (fPushConstant) {
            result += separator + "push_constant";
            separator = ", ";
        }
        switch (fPrimitive) {
            case kPoints_Primitive:
                result += separator + "points";
                separator = ", ";
                break;
            case kLines_Primitive:
                result += separator + "lines";
                separator = ", ";
                break;
            case kLineStrip_Primitive:
                result += separator + "line_strip";
                separator = ", ";
                break;
            case kLinesAdjacency_Primitive:
                result += separator + "lines_adjacency";
                separator = ", ";
                break;
            case kTriangles_Primitive:
                result += separator + "triangles";
                separator = ", ";
                break;
            case kTriangleStrip_Primitive:
                result += separator + "triangle_strip";
                separator = ", ";
                break;
            case kTrianglesAdjacency_Primitive:
                result += separator + "triangles_adjacency";
                separator = ", ";
                break;
            case kUnspecified_Primitive:
                break;
        }
        if (fMaxVertices >= 0) {
            result += separator + "max_vertices = " + to_string(fMaxVertices);
            separator = ", ";
        }
        if (fInvocations >= 0) {
            result += separator + "invocations = " + to_string(fInvocations);
            separator = ", ";
        }
        if (fWhen.size()) {
            result += separator + "when = " + fWhen;
            separator = ", ";
        }
        switch (fKey) {
            case kNo_Key:
                break;
            case kKey_Key:
                result += separator + "key";
                separator = ", ";
                break;
            case kIdentity_Key:
                result += separator + "key=identity";
                separator = ", ";
                break;
        }
        if (result.size() > 0) {
            result = "layout (" + result + ")";
        }
        return result;
    }

    bool operator==(const Layout& other) const {
        return fLocation                 == other.fLocation &&
               fOffset                   == other.fOffset &&
               fBinding                  == other.fBinding &&
               fIndex                    == other.fIndex &&
               fSet                      == other.fSet &&
               fBuiltin                  == other.fBuiltin &&
               fInputAttachmentIndex     == other.fInputAttachmentIndex &&
               fOriginUpperLeft          == other.fOriginUpperLeft &&
               fOverrideCoverage         == other.fOverrideCoverage &&
               fBlendSupportAllEquations == other.fBlendSupportAllEquations &&
               fFormat                   == other.fFormat &&
               fPrimitive                == other.fPrimitive &&
               fMaxVertices              == other.fMaxVertices &&
               fInvocations              == other.fInvocations;
    }

    bool operator!=(const Layout& other) const {
        return !(*this == other);
    }

    int fLocation;
    int fOffset;
    int fBinding;
    int fIndex;
    int fSet;
    // builtin comes from SPIR-V and identifies which particular builtin value this object
    // represents.
    int fBuiltin;
    // input_attachment_index comes from Vulkan/SPIR-V to connect a shader variable to the a
    // corresponding attachment on the subpass in which the shader is being used.
    int fInputAttachmentIndex;
    bool fOriginUpperLeft;
    bool fOverrideCoverage;
    bool fBlendSupportAllEquations;
    Format fFormat;
    bool fPushConstant;
    Primitive fPrimitive;
    int fMaxVertices;
    int fInvocations;
    String fWhen;
    Key fKey;
};

} // namespace

#endif
