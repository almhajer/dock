#version 450

// إحداثيات النسيج من الفيرتكس شيدر
layout(location = 0) in vec2 fragTexCoord;

// نسيج السبرايت
layout(binding = 0) uniform sampler2D texSampler;

// لون البكسل النهائي
layout(location = 0) out vec4 outColor;

void main() {
    vec4 texColor = texture(texSampler, fragTexCoord);

    // discard البكسلات الشفافة تمامًا
    if (texColor.a < 0.01) {
        discard;
    }

    outColor = texColor;
}
