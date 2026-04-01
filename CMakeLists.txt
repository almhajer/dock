cmake_minimum_required(VERSION 3.20)
project(DuckH VERSION 0.1.0 LANGUAGES CXX)

set(MACOS_APP_DISPLAY_NAME "Duck")
set(MACOS_APP_BUNDLE_ID "com.duck.app")
set(MACOS_APP_ICON_FILE "DuckHunterStarter.icns")

include(GNUInstallDirs)

# تفعيل ++C الحديثة مع تعطيل الامتدادات الخاصة بالمترجم.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DUCK_ENABLE_WARNINGS "Enable strict warnings" ON)

if(DUCK_ENABLE_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
        add_compile_options(-Wall -Wextra -Wpedantic)
    endif()
endif()

# جمع ملفات المصدر تلقائيًا حتى لا نعدّل القائمة يدويًا عند إضافة ملفات جديدة.
# الخيار CONFIGURE_DEPENDS يجعل CMake يعيد الفحص عند تغير الملفات.
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

set(MACOS_BUNDLE_INFO_PLIST_FILE "")
set(MACOS_BUNDLE_ICON_SOURCE "")
if(APPLE)
    # إعداد ملف Info.plist وأيقونة التطبيق داخل حزمة macOS.
    set(MACOS_BUNDLE_ICON_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/macos/DuckHunterStarter.icns")
    set(MACOS_BUNDLE_INFO_PLIST_FILE "${CMAKE_CURRENT_BINARY_DIR}/DuckHunterStarter-Info.plist")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in"
        "${MACOS_BUNDLE_INFO_PLIST_FILE}"
        @ONLY
    )
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_SOURCES})
else()
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

# إتاحة ترويسات المشروع المحلية لكل ملفات اللعبة.
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(APPLE)
    # وضع الأيقونة داخل Resources وربط خصائص الحزمة الخاصة بتطبيقات macOS.
    if(EXISTS "${MACOS_BUNDLE_ICON_SOURCE}")
        set_source_files_properties("${MACOS_BUNDLE_ICON_SOURCE}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
        target_sources(${PROJECT_NAME} PRIVATE "${MACOS_BUNDLE_ICON_SOURCE}")
    endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${MACOS_BUNDLE_INFO_PLIST_FILE}"
        MACOSX_BUNDLE_BUNDLE_NAME "${MACOS_APP_DISPLAY_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${MACOS_APP_BUNDLE_ID}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "${MACOS_APP_ICON_FILE}"
    )
endif()

# على macOS نعتمد على /usr/local لأن Vulkan و GLFW موجودتان هناك على هذا الجهاز.
set(VULKAN_MACOS_HINTS)
set(GLFW_MACOS_HINTS)
set(GLFW_MACOS_LIBRARY_SUFFIXES lib)
if(APPLE)
    foreach(SYSTEM_PREFIX IN ITEMS "/usr/local")
        list(APPEND VULKAN_MACOS_HINTS
            "${SYSTEM_PREFIX}"
            "${SYSTEM_PREFIX}/vulkan"
        )
        list(APPEND GLFW_MACOS_HINTS
            "${SYSTEM_PREFIX}"
            "${SYSTEM_PREFIX}/glfw"
        )
    endforeach()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
        list(APPEND GLFW_MACOS_LIBRARY_SUFFIXES lib-arm64 lib-universal)
    else()
        list(APPEND GLFW_MACOS_LIBRARY_SUFFIXES lib-x86_64 lib-universal)
    endif()

    list(REMOVE_DUPLICATES VULKAN_MACOS_HINTS)
    list(REMOVE_DUPLICATES GLFW_MACOS_HINTS)
    list(REMOVE_DUPLICATES GLFW_MACOS_LIBRARY_SUFFIXES)
endif()

# نفضّل اكتشاف المكتبات من النظام أولًا، ثم ننتقل لمسارات macOS المعروفة.
find_package(Vulkan QUIET)

set(VULKAN_FALLBACK_INCLUDE_DIR "")
set(VULKAN_FALLBACK_LIBRARY "")
if(NOT Vulkan_FOUND AND APPLE)
    find_path(VULKAN_FALLBACK_INCLUDE_DIR
        NAMES vulkan/vulkan.h
        PATHS ${VULKAN_MACOS_HINTS}
        PATH_SUFFIXES include
    )
    find_library(VULKAN_FALLBACK_LIBRARY
        NAMES vulkan Vulkan MoltenVK
        PATHS ${VULKAN_MACOS_HINTS}
        PATH_SUFFIXES lib
    )

    if(VULKAN_FALLBACK_INCLUDE_DIR AND VULKAN_FALLBACK_LIBRARY)
        add_library(Vulkan::Vulkan UNKNOWN IMPORTED)
        set_target_properties(Vulkan::Vulkan PROPERTIES
            IMPORTED_LOCATION "${VULKAN_FALLBACK_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${VULKAN_FALLBACK_INCLUDE_DIR}"
        )
        set(Vulkan_FOUND TRUE)
    endif()
endif()

if(NOT Vulkan_FOUND)
    message(FATAL_ERROR
        "Vulkan was not found. Install the Vulkan SDK or MoltenVK, or expose it to CMake."
    )
endif()

find_package(glfw3 QUIET)

set(GLFW_TARGET "")
if(TARGET glfw)
    set(GLFW_TARGET glfw)
elseif(TARGET glfw3)
    set(GLFW_TARGET glfw3)
else()
    set(GLFW_FALLBACK_LIBRARY "")
    set(GLFW_FALLBACK_INCLUDE_DIR "")

    if(APPLE)
        # بعض تثبيتات GLFW على macOS تضع المكتبة داخل مجلدات معمارية مثل lib-arm64.
        foreach(GLFW_HINT IN LISTS GLFW_MACOS_HINTS)
            if(NOT GLFW_FALLBACK_INCLUDE_DIR AND EXISTS "${GLFW_HINT}/include/GLFW/glfw3.h")
                set(GLFW_FALLBACK_INCLUDE_DIR "${GLFW_HINT}/include")
            endif()

            foreach(GLFW_LIB_SUFFIX IN LISTS GLFW_MACOS_LIBRARY_SUFFIXES)
                foreach(GLFW_LIB_NAME IN ITEMS libglfw.dylib libglfw.3.dylib libglfw3.a)
                    if(NOT GLFW_FALLBACK_LIBRARY AND EXISTS "${GLFW_HINT}/${GLFW_LIB_SUFFIX}/${GLFW_LIB_NAME}")
                        set(GLFW_FALLBACK_LIBRARY "${GLFW_HINT}/${GLFW_LIB_SUFFIX}/${GLFW_LIB_NAME}")
                    endif()
                endforeach()
            endforeach()
        endforeach()
    else()
        find_library(GLFW_FALLBACK_LIBRARY NAMES glfw glfw3)
        find_path(GLFW_FALLBACK_INCLUDE_DIR NAMES GLFW/glfw3.h)
    endif()

    if(GLFW_FALLBACK_LIBRARY AND GLFW_FALLBACK_INCLUDE_DIR)
        add_library(glfw UNKNOWN IMPORTED)
        set_target_properties(glfw PROPERTIES
            IMPORTED_LOCATION "${GLFW_FALLBACK_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GLFW_FALLBACK_INCLUDE_DIR}"
        )
        set(GLFW_TARGET glfw)
    endif()
endif()

if(NOT GLFW_TARGET)
    message(FATAL_ERROR
        "GLFW3 was not found. Install glfw and make it visible to CMake."
    )
endif()

if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreFoundation"
            "-framework CoreGraphics"
            "-framework ImageIO"
            "-framework QuartzCore"
    )
endif()

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        Vulkan::Vulkan
        ${GLFW_TARGET}
)

if(APPLE)
    # نسخ الأصول داخل الحزمة حتى تبقى المسارات المحلية متاحة بعد نقل التطبيق.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/assets"
            COMMENT "نسخ أصول اللعبة داخل حزمة تطبيق macOS"
        )
    endif()

    # تثبيت الحزمة كبرنامج مستقل مع الأصول داخل Resources.
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            DESTINATION "${PROJECT_NAME}.app/Contents/Resources"
        )
    endif()

    # بعد التثبيت ننسخ المكتبات الديناميكية المطلوبة إلى داخل الحزمة ونعدّل المسارات.
    install(CODE "
        include(BundleUtilities)
        set(BU_CHMOD_BUNDLE_ITEMS ON)
        fixup_bundle(
            \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\"
            \"\"
            \"/usr/local/lib;/usr/local/glfw/lib-arm64;/usr/local/glfw/lib-universal;/usr/local/glfw/lib-x86_64\"
        )
    ")
else()
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            DESTINATION "${CMAKE_INSTALL_BINDIR}"
        )
    endif()
endif()
