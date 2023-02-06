/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 * Copyright (C) 2016-2023 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "CSSPrimitiveValue.h"
#include "CSSValueKeywords.h"
#include "Color.h"
#include "ColorInterpolationMethod.h"
#include <wtf/Ref.h>
#include <wtf/UniqueRef.h>

namespace WebCore {

enum class StyleColorOptions : uint8_t {
    ForVisitedLink                 = 1 << 0,
    UseSystemAppearance            = 1 << 1,
    UseDarkAppearance              = 1 << 2,
    UseElevatedUserInterfaceLevel  = 1 << 3
};

struct StyleColorMix;
struct StyleCurrentColor { };

class ExtendedStyleColor {
public:
    using Kind = std::variant<UniqueRef<StyleColorMix>, StyleCurrentColor>;
    ExtendedStyleColor(StyleColorMix);
    ExtendedStyleColor(StyleCurrentColor);
    Kind m_color;
};

class StyleColor {
public:
    // The default constructor initializes to currentcolor to preserve old behavior,
    // we might want to change it to invalid color at some point.
    StyleColor();
    StyleColor(const Color& color);
    StyleColor(const SRGBA<uint8_t>& color);
    StyleColor(const StyleColor& other);
    StyleColor& operator=(const StyleColor& other);
    explicit StyleColor(const ExtendedStyleColor& color);

    /*
    StyleColor(StyleColorMix&& colorMix)
        : m_color { resolveAbsoluteComponents(WTFMove(colorMix)) }
    {
    }
    */

    StyleColor(StyleColor&&) = default;
    StyleColor& operator=(StyleColor&&) = default;
    StyleColor(Color&& color);

    WEBCORE_EXPORT ~StyleColor();

    static StyleColor currentColor() { 
        return { };
    }

    static Color colorFromKeyword(CSSValueID, OptionSet<StyleColorOptions>);
    static Color colorFromAbsoluteKeyword(CSSValueID);

    static bool containsCurrentColor(const CSSPrimitiveValue&);
    static bool isAbsoluteColorKeyword(CSSValueID);
    static bool isCurrentColorKeyword(CSSValueID id) { return id == CSSValueCurrentcolor; }
    static bool isCurrentColor(const CSSPrimitiveValue& value) { return isCurrentColorKeyword(value.valueID()); }
    const ExtendedStyleColor& asExtendedStyleColor() const;
    bool isExtendedStyleColor() const;
    void setExtendedStyleColor(const ExtendedStyleColor&);


    WEBCORE_EXPORT static bool isSystemColorKeyword(CSSValueID);
    static bool isDeprecatedSystemColorKeyword(CSSValueID);

    enum class CSSColorType : uint8_t {
        Absolute =    1 << 0,
        Current =     1 << 1,
        System =      1 << 2,
    };

    // https://drafts.csswg.org/css-color-4/#typedef-color
    static bool isColorKeyword(CSSValueID, OptionSet<CSSColorType> = { CSSColorType::Absolute, CSSColorType::Current, CSSColorType::System });

    bool containsCurrentColor() const;
    bool isCurrentColor() const;
    bool isColorMix() const;
    bool isAbsoluteColor() const;
    const Color& absoluteColor() const;

    WEBCORE_EXPORT Color resolveColor(const Color& colorPropertyValue) const;

    bool operator==(const StyleColor&) const = default;
    friend WEBCORE_EXPORT String serializationForCSS(const StyleColor&);
    friend void serializationForCSS(StringBuilder&, const StyleColor&);
    friend WTF::TextStream& operator<<(WTF::TextStream&, const StyleColor&);
    String debugDescription() const;

private:
    Color m_color;
};

struct StyleColorMix {
    WTF_MAKE_STRUCT_FAST_ALLOCATED;

    struct Component {
        StyleColor color;
        std::optional<double> percentage;
        bool operator==(const StyleColorMix::Component&) const = default;
    };

    bool operator==(const StyleColorMix&) const = default;
    ColorInterpolationMethod colorInterpolationMethod;
    Component mixComponents1;
    Component mixComponents2;
};

inline bool operator==(const UniqueRef<StyleColorMix>& a, const UniqueRef<StyleColorMix>& b)
{
    return a.get() == b.get();
}

constexpr bool operator==(const StyleCurrentColor&, const StyleCurrentColor&)
{
    return true;
}

WTF::TextStream& operator<<(WTF::TextStream&, const ExtendedStyleColor&);
WTF::TextStream& operator<<(WTF::TextStream&, const StyleColorMix&);
WTF::TextStream& operator<<(WTF::TextStream&, const StyleCurrentColor&);
WTF::TextStream& operator<<(WTF::TextStream&, const StyleColor&);

void serializationForCSS(StringBuilder&, const ExtendedStyleColor&);
void serializationForCSS(StringBuilder&, const StyleColorMix&);
void serializationForCSS(StringBuilder&, const StyleCurrentColor&);
void serializationForCSS(StringBuilder&, const StyleColor&);

template <class T, class... Ts>
struct is_any : std::disjunction<std::is_same<T, Ts>...> {};

template <typename T, std::enable_if_t<is_any<T, ExtendedStyleColor,StyleColorMix,StyleCurrentColor,StyleColor>::value>* = 0>
WEBCORE_EXPORT String serializationForCSS(const T& color)
{
    StringBuilder builder;
    serializationForCSS(builder, color);
    return builder.toString();
}

} // namespace WebCore
