#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace gfx {

/// @brief نوع مادة quad القياسي عند استخدام خامة عادية.
inline constexpr float QUAD_MATERIAL_TEXTURED = 0.0f;

/// @brief نوع مادة quad عند رسم العشب الإجرائي.
inline constexpr float QUAD_MATERIAL_PROCEDURAL_GRASS = 1.0f;

/// @brief نوع مادة quad عند رسم التربة الإجرائية.
inline constexpr float QUAD_MATERIAL_PROCEDURAL_SOIL = 2.0f;

/// @brief عدد التقسيمات العمودية لكل quad لدعم تأثيرات الانحناء والحركة.
inline constexpr std::size_t QUAD_VERTICAL_SEGMENTS = 6;

/// @brief عدد الرؤوس الفعلي الناتج من تقسيم quad إلى مثلثات.
inline constexpr std::size_t QUAD_VERTEX_COUNT = QUAD_VERTICAL_SEGMENTS * 6;

/// @brief مستطيل إحداثيات UV داخل الخامة.
struct UvRect {
    /// @brief بداية الإحداثي الأفقي داخل الخامة.
    float u0 = 0.0f;

    /// @brief نهاية الإحداثي الأفقي داخل الخامة.
    float u1 = 0.0f;

    /// @brief بداية الإحداثي العمودي داخل الخامة.
    float v0 = 0.0f;

    /// @brief نهاية الإحداثي العمودي داخل الخامة.
    float v1 = 0.0f;
};

/// @brief مستطيل الموضع على الشاشة في الفضاء المنطقي.
struct ScreenRect {
    /// @brief الحد الأيسر للمستطيل.
    float x0 = 0.0f;

    /// @brief الحد الأيمن للمستطيل.
    float x1 = 0.0f;

    /// @brief الحد العلوي للمستطيل.
    float y0 = 0.0f;

    /// @brief الحد السفلي للمستطيل.
    float y1 = 0.0f;
};

/// @brief quad مرسوم بخامة واحدة مع خصائص الشفافية والرياح والدوران.
struct TexturedQuad {
    /// @brief أبعاد quad على الشاشة.
    ScreenRect screen;

    /// @brief مستطيل UV المستخدم من الخامة.
    UvRect uv;

    /// @brief قيمة الشفافية النهائية.
    float alpha = 1.0f;

    /// @brief وزن تأثير الرياح على هذا quad.
    float windWeight = 0.0f;

    /// @brief طور حركة الرياح لهذا العنصر.
    float windPhase = 0.0f;

    /// @brief مدى استجابة العنصر للرياح.
    float windResponse = 0.0f;

    /// @brief نوع المادة المستخدمة داخل الشيدر.
    float materialType = QUAD_MATERIAL_TEXTURED;

    /// @brief زاوية الدوران بالراديان.
    float rotationRadians = 0.0f;
};

/// @brief رأس واحد داخل المثلثات الناتجة من quad.
struct QuadVertex {
    /// @brief الإحداثي الأفقي للرأس.
    float x = 0.0f;

    /// @brief الإحداثي العمودي للرأس.
    float y = 0.0f;

    /// @brief إحداثي UV الأفقي للرأس.
    float u = 0.0f;

    /// @brief إحداثي UV العمودي للرأس.
    float v = 0.0f;

    /// @brief شفافية هذا الرأس.
    float alpha = 1.0f;

    /// @brief وزن تأثير الرياح على الرأس.
    float windWeight = 0.0f;

    /// @brief طور الرياح للرأس.
    float windPhase = 0.0f;

    /// @brief استجابة الرأس للرياح.
    float windResponse = 0.0f;

    /// @brief نوع المادة التي يفسرها الشيدر.
    float materialType = QUAD_MATERIAL_TEXTURED;
};

/// @brief يرجع مستطيل UV كامل يغطي الخامة كلها.
/// @return مستطيل UV من (0,0) إلى (1,1).
[[nodiscard]] inline constexpr UvRect fullUvRect() {
    return {0.0f, 1.0f, 0.0f, 1.0f};
}

/// @brief يبني مستطيلاً من حدود شاشة مباشرة.
/// @param left الحد الأيسر.
/// @param right الحد الأيمن.
/// @param top الحد العلوي.
/// @param bottom الحد السفلي.
/// @return مستطيل الشاشة.
[[nodiscard]] inline constexpr ScreenRect makeScreenRect(float left, float right, float top, float bottom) {
    return {left, right, top, bottom};
}

/// @brief يبني quad نصي/خامي مع جميع خصائصه الاختيارية.
/// @param screen أبعاد quad على الشاشة.
/// @param uv مستطيل UV المستخدم من الخامة.
/// @param alpha قيمة الشفافية.
/// @param windWeight وزن تأثير الرياح.
/// @param windPhase طور حركة الرياح.
/// @param windResponse مدى استجابة الرياح.
/// @param materialType نوع المادة.
/// @param rotationRadians زاوية الدوران.
/// @return quad جاهز للرسم.
[[nodiscard]] inline constexpr TexturedQuad makeTexturedQuad(
    ScreenRect screen,
    UvRect uv = fullUvRect(),
    float alpha = 1.0f,
    float windWeight = 0.0f,
    float windPhase = 0.0f,
    float windResponse = 0.0f,
    float materialType = QUAD_MATERIAL_TEXTURED,
    float rotationRadians = 0.0f
) {
    return {screen, uv, alpha, windWeight, windPhase, windResponse, materialType, rotationRadians};
}

/// @brief استيفاء خطي بسيط بين قيمتين عشريتين.
/// @param startValue قيمة البداية.
/// @param endValue قيمة النهاية.
/// @param factor معامل الاستيفاء بين 0 و 1.
/// @return القيمة المستوفاة.
[[nodiscard]] inline float lerpFloat(float startValue, float endValue, float factor) {
    return startValue + (endValue - startValue) * factor;
}

/// @brief منحنى ناعم لوزن الرياح عبر تقسيمات quad.
/// @param t الموضع النسبي بين 0 و 1.
/// @return وزن الرياح الملساء.
[[nodiscard]] inline float easedWindWeight(float t) {
    return t * t * (3.0f - 2.0f * t);
}

/// @brief يكتب رؤوس quad المقسم في مصفوفة الرؤوس الجاهزة للرسم.
/// @param quad quad المراد تحويله إلى رؤوس.
/// @param vertices مؤشر إلى مصفوفة الرؤوس بحجم QUAD_VERTEX_COUNT على الأقل.
inline void writeQuadVertices(const TexturedQuad& quad, QuadVertex* vertices) {
    std::size_t vertexIndex = 0;
    const float centerX = (quad.screen.x0 + quad.screen.x1) * 0.5f;
    const float centerY = (quad.screen.y0 + quad.screen.y1) * 0.5f;
    const float cosAngle = std::cos(quad.rotationRadians);
    const float sinAngle = std::sin(quad.rotationRadians);

    const auto rotatePoint = [&](float x, float y) {
        const float localX = x - centerX;
        const float localY = y - centerY;
        return std::array<float, 2>{
            centerX + localX * cosAngle - localY * sinAngle,
            centerY + localX * sinAngle + localY * cosAngle,
        };
    };

    for (std::size_t segment = 0; segment < QUAD_VERTICAL_SEGMENTS; ++segment) {
        const float topT = static_cast<float>(segment) / static_cast<float>(QUAD_VERTICAL_SEGMENTS);
        const float bottomT = static_cast<float>(segment + 1) / static_cast<float>(QUAD_VERTICAL_SEGMENTS);

        const float yTop = lerpFloat(quad.screen.y0, quad.screen.y1, topT);
        const float yBottom = lerpFloat(quad.screen.y0, quad.screen.y1, bottomT);
        const float vTop = lerpFloat(quad.uv.v0, quad.uv.v1, topT);
        const float vBottom = lerpFloat(quad.uv.v0, quad.uv.v1, bottomT);

        const float topWeight = quad.windWeight * easedWindWeight(1.0f - topT);
        const float bottomWeight = quad.windWeight * easedWindWeight(1.0f - bottomT);

        const auto topLeft = rotatePoint(quad.screen.x0, yTop);
        const auto topRight = rotatePoint(quad.screen.x1, yTop);
        const auto bottomRight = rotatePoint(quad.screen.x1, yBottom);
        const auto bottomLeft = rotatePoint(quad.screen.x0, yBottom);

        vertices[vertexIndex++] = {topLeft[0], topLeft[1], quad.uv.u0, vTop, quad.alpha, topWeight, quad.windPhase, quad.windResponse, quad.materialType};
        vertices[vertexIndex++] = {topRight[0], topRight[1], quad.uv.u1, vTop, quad.alpha, topWeight, quad.windPhase, quad.windResponse, quad.materialType};
        vertices[vertexIndex++] = {bottomRight[0], bottomRight[1], quad.uv.u1, vBottom, quad.alpha, bottomWeight, quad.windPhase, quad.windResponse, quad.materialType};
        vertices[vertexIndex++] = {topLeft[0], topLeft[1], quad.uv.u0, vTop, quad.alpha, topWeight, quad.windPhase, quad.windResponse, quad.materialType};
        vertices[vertexIndex++] = {bottomRight[0], bottomRight[1], quad.uv.u1, vBottom, quad.alpha, bottomWeight, quad.windPhase, quad.windResponse, quad.materialType};
        vertices[vertexIndex++] = {bottomLeft[0], bottomLeft[1], quad.uv.u0, vBottom, quad.alpha, bottomWeight, quad.windPhase, quad.windResponse, quad.materialType};
    }
}

/// @brief يبني جميع رؤوس quad في مصفوفة ثابتة جاهزة.
/// @param quad quad المراد تحويله إلى رؤوس.
/// @return مصفوفة الرؤوس بحجم QUAD_VERTEX_COUNT.
[[nodiscard]] inline std::array<QuadVertex, QUAD_VERTEX_COUNT> buildQuadVertices(const TexturedQuad& quad) {
    std::array<QuadVertex, QUAD_VERTEX_COUNT> vertices{};
    writeQuadVertices(quad, vertices.data());
    return vertices;
}

} // namespace gfx
