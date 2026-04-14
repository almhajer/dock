#include "DockIcon.h"

#include <iostream>

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#endif

namespace core
{

void setDockIcon(const std::string& iconPath)
{
#ifdef __APPLE__
    @autoreleasepool
    {
        /*
         * نحتاج NSApplication فعّال حتى تظهر الأيقونة في الـ Dock
         */
        if (NSApp == nil)
        {
            [NSApplication sharedApplication];
        }

        /*
         * ضبط سياسة التفعيل لضمان ظهور التطبيق في الـ Dock
         */
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        NSImage* icon = nil;

        /*
         * المحاولة الأولى: تحميل من موارد الحزمة (NSBundle)
         * هذا يعمل بشكل أفضل من المسار المباشر على macOS
         */
        NSString* iconName = @"DuckHunterStarter";
        icon = [NSImage imageNamed:iconName];

        if (icon == nil)
        {
            /*
             * المحاولة الثانية: تحميل .icns من مجلد Resources
             */
            NSString* icnsPath = [[NSBundle mainBundle] pathForResource:@"DuckHunterStarter"
                                                                ofType:@"icns"];
            if (icnsPath != nil)
            {
                icon = [[NSImage alloc] initWithContentsOfFile:icnsPath];
            }
        }

        if (icon == nil)
        {
            /*
             * المحاولة الثالثة: تحميل من المسار المباشر
             */
            NSString* nsPath = [NSString stringWithUTF8String:iconPath.c_str()];
            icon = [[NSImage alloc] initWithContentsOfFile:nsPath];
        }

        if (icon == nil)
        {
            /*
             * المحاولة الرابعة: تحميل .icns من مسار الأصول
             */
            NSString* basePath = [NSString stringWithUTF8String:iconPath.c_str()];
            NSString* icnsFromAssets = [[basePath stringByDeletingPathExtension] stringByAppendingPathExtension:@"icns"];
            icon = [[NSImage alloc] initWithContentsOfFile:icnsFromAssets];
        }

        if (icon != nil)
        {
            [NSApp setApplicationIconImage:icon];
            [NSApp activateIgnoringOtherApps:YES];

            std::cout << "[DockIcon] تم ضبط أيقونة التطبيق (حجم: "
                      << static_cast<int>(icon.size.width) << "x"
                      << static_cast<int>(icon.size.height) << ")" << std::endl;
        }
        else
        {
            std::cerr << "[DockIcon] تعذر تحميل الأيقونة من أي مصدر" << std::endl;
        }
    }
#else
    (void)iconPath;
#endif
}

} // namespace core
