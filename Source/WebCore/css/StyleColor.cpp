/*
 * Copyright (C) 2015 Google Inc. All rights reserved.
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

#include "config.h"
#include "StyleColor.h"

#include "CSSResolvedColorMix.h"
#include "CSSUnresolvedColor.h"
#include "ColorNormalization.h"
#include "ColorSerialization.h"
#include "HashTools.h"
#include "RenderTheme.h"
#include <wtf/text/TextStream.h>

namespace WebCore {

//static std::optional<Color> resolveAbsoluteComponents(const StyleColorMix&);
static Color resolveColor(const StyleColorMix&, const Color& currentColor);

bool StyleColor::isAbsoluteColor() const
{
    return !isExtendedStyleColor();
}

bool StyleColor::isExtendedStyleColor() const
{
    return m_color.flags().contains(Color::FlagsIncludingPrivate::ExtendedStyleColor);
}

void StyleColor::setExtendedStyleColor(const ExtendedStyleColor& color)
{
    auto flags = { Color::FlagsIncludingPrivate::Valid, Color::FlagsIncludingPrivate::ExtendedStyleColor};
    m_color.m_colorAndFlags = Color::encodedPointer(static_cast<const void*>(&color)) | Color::encodedFlags(flags);
    ASSERT(isExtendedStyleColor());
}

const ExtendedStyleColor& StyleColor::asExtendedStyleColor() const
{
    ASSERT(isExtendedStyleColor());
    return *static_cast<ExtendedStyleColor*>(Color::decodedPointer(m_color.m_colorAndFlags));
}

StyleColor::StyleColor()
{
    auto currentColor = new ExtendedStyleColor { StyleCurrentColor { } };
    setExtendedStyleColor(*currentColor);
}

StyleColor::StyleColor(const Color& color)
    : m_color {  color }
{
}

StyleColor::StyleColor(const SRGBA<uint8_t>& color)
    : m_color { color }
{
}

StyleColor::StyleColor(const ExtendedStyleColor& color)
{
    setExtendedStyleColor(color);
}

StyleColor::StyleColor(const StyleColor& other)
    : m_color { other.m_color }
{
}

StyleColor::StyleColor(Color&& color)
    : m_color { WTFMove(color) }
{
}

StyleColor& StyleColor::operator=(const StyleColor& other)
{
    m_color = other.m_color;
    return *this;
}

StyleColor::~StyleColor()
{
    /*
    if (isExtendedStyleColor())
        delete &asExtendedStyleColor();
    */
}

ExtendedStyleColor::ExtendedStyleColor(StyleColorMix color)
: m_color(makeUniqueRef<StyleColorMix>(color))
{
    
}

ExtendedStyleColor::ExtendedStyleColor(StyleCurrentColor color)
: m_color(color)
{
    
}

Color StyleColor::colorFromAbsoluteKeyword(CSSValueID keyword)
{
    // TODO: maybe it should be a constexpr map for performance.
    ASSERT(StyleColor::isAbsoluteColorKeyword(keyword));
    if (const char* valueName = nameLiteral(keyword)) {
        if (auto namedColor = findColor(valueName, strlen(valueName)))
            return asSRGBA(PackedColor::ARGB { namedColor->ARGBValue });
    }
    ASSERT_NOT_REACHED();
    return { };
}

Color StyleColor::colorFromKeyword(CSSValueID keyword , OptionSet<StyleColorOptions> options)
{
    if (isAbsoluteColorKeyword(keyword))
        return colorFromAbsoluteKeyword(keyword);

    return RenderTheme::singleton().systemColor(keyword, options);
}

static bool isVGAPaletteColor(CSSValueID id)
{
    // https://drafts.csswg.org/css-color-4/#named-colors
    // "16 of CSS’s named colors come from the VGA palette originally, and were then adopted into HTML"
    return (id >= CSSValueAqua && id <= CSSValueGrey);
}

static bool isNonVGANamedColor(CSSValueID id)
{
    // https://drafts.csswg.org/css-color-4/#named-colors
    return id >= CSSValueAliceblue && id <= CSSValueYellowgreen;
}

bool StyleColor::isAbsoluteColorKeyword(CSSValueID id)
{
    // https://drafts.csswg.org/css-color-4/#typedef-absolute-color
    return isVGAPaletteColor(id) || isNonVGANamedColor(id) || id == CSSValueAlpha || id == CSSValueTransparent;
}

bool StyleColor::isSystemColorKeyword(CSSValueID id)
{
    // https://drafts.csswg.org/css-color-4/#css-system-colors
    return (id >= CSSValueCanvas && id <= CSSValueInternalDocumentTextColor) || id == CSSValueText || isDeprecatedSystemColorKeyword(id);
}

bool StyleColor::isDeprecatedSystemColorKeyword(CSSValueID id)
{
    // https://drafts.csswg.org/css-color-4/#deprecated-system-colors
    return (id >= CSSValueActiveborder && id <= CSSValueWindowtext) || id == CSSValueMenu;
}

bool StyleColor::isColorKeyword(CSSValueID id, OptionSet<CSSColorType> allowedColorTypes)
{
    return (allowedColorTypes.contains(CSSColorType::Absolute) && isAbsoluteColorKeyword(id))
        || (allowedColorTypes.contains(CSSColorType::Current) && isCurrentColorKeyword(id))
        || (allowedColorTypes.contains(CSSColorType::System) && isSystemColorKeyword(id));
}

bool StyleColor::containsCurrentColor(const CSSPrimitiveValue& value)
{
    if (StyleColor::isCurrentColor(value))
        return true;

    if (value.isUnresolvedColor())
        return value.unresolvedColor().containsCurrentColor();

    return false;
}

String StyleColor::debugDescription() const
{
    TextStream ts;
    ts << *this;
    return ts.release();
}

Color StyleColor::resolveColor(const Color& currentColor) const
{
    if (isAbsoluteColor())
        return m_color;

    return WTF::switchOn(asExtendedStyleColor().m_color,
        [&] (const StyleCurrentColor&) -> Color {
            return currentColor;
        },
        [&] (const UniqueRef<StyleColorMix>& colorMix) -> Color {
            return WebCore::resolveColor(colorMix, currentColor);
        }
    );
}

bool StyleColor::containsCurrentColor() const
{
    if (isAbsoluteColor())
        return false;

    return WTF::switchOn(asExtendedStyleColor().m_color,
        [&] (const StyleCurrentColor&) -> bool {
            return true;
        },
        [&] (const UniqueRef<StyleColorMix>& colorMix) -> bool {
            return colorMix->mixComponents1.color.containsCurrentColor() || colorMix->mixComponents2.color.containsCurrentColor();
        }
    );
}

bool StyleColor::isCurrentColor() const
{
    if (isAbsoluteColor())
        return false;

    return std::holds_alternative<StyleCurrentColor>(asExtendedStyleColor().m_color);
}

bool StyleColor::isColorMix() const
{
    if (isAbsoluteColor())
        return false;

    return std::holds_alternative<UniqueRef<StyleColorMix>>(asExtendedStyleColor().m_color);
}

const Color& StyleColor::absoluteColor() const
{
    ASSERT(isAbsoluteColor());
    return m_color;
}

/*
Color StyleColor::resolveAbsoluteComponents(StyleColorMix&& colorMix)
{
    if (auto absoluteColor = WebCore::resolveAbsoluteComponents(colorMix))
        return *absoluteColor;
    return { };
}
*/

// MARK: color-mix()

/*
std::optional<Color> resolveAbsoluteComponents(const StyleColorMix& colorMix)
{
    if (!colorMix.mixComponents1.color.isAbsoluteColor() || !colorMix.mixComponents2.color.isAbsoluteColor())
        return std::nullopt;

    return mix(CSSResolvedColorMix {
        colorMix.colorInterpolationMethod,
        CSSResolvedColorMix::Component {
            colorMix.mixComponents1.color.absoluteColor(),
            colorMix.mixComponents1.percentage
        },
        CSSResolvedColorMix::Component {
            colorMix.mixComponents2.color.absoluteColor(),
            colorMix.mixComponents2.percentage
        }
    });
}
*/

Color resolveColor(const StyleColorMix& colorMix, const Color& currentColor)
{
    return mix(CSSResolvedColorMix {
        colorMix.colorInterpolationMethod,
        CSSResolvedColorMix::Component {
            colorMix.mixComponents1.color.resolveColor(currentColor),
            colorMix.mixComponents1.percentage
        },
        CSSResolvedColorMix::Component {
            colorMix.mixComponents2.color.resolveColor(currentColor),
            colorMix.mixComponents2.percentage
        }
    });
}

// MARK: - Serialization

static void serializationForCSS(StringBuilder& builder, const StyleColorMix::Component& component)
{
    serializationForCSS(builder, component.color);
    if (component.percentage)
        builder.append(' ', *component.percentage, '%');
}

void serializationForCSS(StringBuilder& builder, const StyleColorMix& colorMix)
{
    builder.append("color-mix(in ");
    serializationForCSS(builder, colorMix.colorInterpolationMethod);
    builder.append(", ");
    serializationForCSS(builder, colorMix.mixComponents1);
    builder.append(", ");
    serializationForCSS(builder, colorMix.mixComponents2);
    builder.append(')');
}

void serializationForCSS(StringBuilder& builder, const StyleCurrentColor&)
{
    builder.append("currentcolor"_s);
}

void serializationForCSS(StringBuilder& builder, const StyleColor& color)
{
    if (color.isAbsoluteColor())
        return serializationForCSS(builder, color.absoluteColor());

    serializationForCSS(builder, color.asExtendedStyleColor());
}

void serializationForCSS(StringBuilder& builder, const ExtendedStyleColor& color)
{
    WTF::switchOn(color.m_color,
        [&] (const StyleCurrentColor& currentColor) {
            serializationForCSS(builder, currentColor);
        },
        [&] (const UniqueRef<StyleColorMix>& colorMix) {
            serializationForCSS(builder, colorMix);
        }
    );
}

String serializationForCSS(const StyleColor& color)
{
    StringBuilder builder;
    serializationForCSS(builder, color);
    return builder.toString();
}

// MARK: - TextStream.

static TextStream& operator<<(TextStream& ts, const StyleColorMix::Component& component)
{
    ts << component.color;
    if (component.percentage)
        ts << " " << *component.percentage << "%";
    return ts;
}

TextStream& operator<<(TextStream& ts, const StyleColorMix& colorMix)
{
    ts << "color-mix(";
    ts << "in " << colorMix.colorInterpolationMethod;
    ts << ", " << colorMix.mixComponents1;
    ts << ", " << colorMix.mixComponents2;
    ts << ")";

    return ts;
}

WTF::TextStream& operator<<(WTF::TextStream& out, const StyleCurrentColor&)
{
    out << "currentColor";
    return out;
}

WTF::TextStream& operator<<(WTF::TextStream& out, const ExtendedStyleColor& color)
{
    WTF::switchOn(color.m_color,
        [&] (const StyleCurrentColor& currentColor) {
            out << currentColor;
        },
        [&] (const UniqueRef<StyleColorMix>& colorMix) {
            out << colorMix.get();
        }
    );
    return out;
}

WTF::TextStream& operator<<(WTF::TextStream& out, const StyleColor& color)
{
    out << "StyleColor[";

    if (color.isAbsoluteColor()) {
        out << "absoluteColor(" << color.absoluteColor().debugDescription() << ")";
    } else {
        out << color.asExtendedStyleColor();
    }

    out << "]";
    return out;
}

} // namespace WebCore
