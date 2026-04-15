/**
 * @file DockIcon.h
 * @brief واجهة أيقونة الdock.
 * @details تعيين أيقونة التطبيق في شريط Dock على macOS.
 */

#pragma once

#include <string>

namespace core
{

/*
 يعيّن أيقونة التطبيق في شريط Dock عبر NSApplication.
 */
void setDockIcon(const std::string& icnsPath);

} // namespace core
