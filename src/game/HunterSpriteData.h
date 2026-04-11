#pragma once

namespace game
{

#pragma region TimingTypes
/*
 توقيتات حركة الصياد المستخدمة فعليًا داخل التطبيق.
 */
struct HunterActionTiming
{
    /*
     @brief مدة إعادة التلقيم اليدوية بالثواني.
     */
    float reloadDurationSeconds = 0.0f;

    /*
     @brief مدة التثبيت على فريم الطلقة الأولى بعد الإطلاق.
     */
    float shotRecoveryDurationSeconds = 0.0f;
};
#pragma endregion TimingTypes

#pragma region TimingAccess
/*
 الوصول إلى توقيتات الإطلاق وإعادة التعبئة.
 */
[[nodiscard]] const HunterActionTiming& hunterActionTiming();
#pragma endregion TimingAccess

} // namespace game
