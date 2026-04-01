#version 450

// موقع الرأس في مساحة الشاشة (-1 إلى 1)
layout(location = 0) in vec2 inPosition;

// إحداثيات النسيج (UV)
layout(location = 1) in vec2 inTexCoord;

// تمرير UV للفراغمنت شيدر
layout(location = 0) out vec2 fragTexCoord;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragTexCoord = inTexCoord;
}
