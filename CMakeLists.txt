cmake_minimum_required(VERSION 3.20)

if(APPLE AND NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING
        "Architectures to build for macOS distributions"
        FORCE
    )
endif()

if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING
        "Minimum macOS version supported by the distributed app"
        FORCE
    )
endif()

project(Duck VERSION 0.1.0 LANGUAGES CXX)

set(MACOS_APP_DISPLAY_NAME "Duck")
set(MACOS_APP_BUNDLE_ID "com.duck.app")
set(MACOS_APP_ICON_FILE "DuckHunterStarter")

include(GNUInstallDirs)

# تفعيل ++C الحديثة مع تعطيل الامتدادات الخاصة بالمترجم.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DUCK_ENABLE_WARNINGS "Enable strict warnings" ON)
option(DUCK_ENABLE_VULKAN_VALIDATION "Enable Vulkan validation layers at runtime" OFF)
set(DUCK_CODESIGN_IDENTITY "-" CACHE STRING
    "macOS codesign identity used for the installed app bundle. Use '-' for ad-hoc signing or empty string to skip signing."
)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

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

if(APPLE)
    set(DUCK_BUNDLE_SEARCH_DIRS "/usr/local/lib")
    set(DUCK_MACOS_WANTS_UNIVERSAL FALSE)
    if(CMAKE_OSX_ARCHITECTURES)
        list(FIND CMAKE_OSX_ARCHITECTURES "arm64" DUCK_ARCH_HAS_ARM64)
        list(FIND CMAKE_OSX_ARCHITECTURES "x86_64" DUCK_ARCH_HAS_X86_64)
        if(DUCK_ARCH_HAS_ARM64 GREATER_EQUAL 0 AND DUCK_ARCH_HAS_X86_64 GREATER_EQUAL 0)
            set(DUCK_MACOS_WANTS_UNIVERSAL TRUE)
        endif()
    endif()
    if(DUCK_MACOS_WANTS_UNIVERSAL)
        list(APPEND DUCK_BUNDLE_SEARCH_DIRS
            "/usr/local/glfw/lib-universal"
            "/usr/local/glfw/lib-arm64"
            "/usr/local/glfw/lib-x86_64"
        )
    else()
        list(APPEND DUCK_BUNDLE_SEARCH_DIRS
            "/usr/local/glfw/lib-arm64"
            "/usr/local/glfw/lib-universal"
            "/usr/local/glfw/lib-x86_64"
        )
    endif()

    set(MOLTENVK_DYLIB_SOURCE "")
    foreach(SYSTEM_PREFIX IN ITEMS "/usr/local")
        if(NOT MOLTENVK_DYLIB_SOURCE AND EXISTS "${SYSTEM_PREFIX}/lib/libMoltenVK.dylib")
            set(MOLTENVK_DYLIB_SOURCE "${SYSTEM_PREFIX}/lib/libMoltenVK.dylib")
        endif()
    endforeach()
    if(MOLTENVK_DYLIB_SOURCE)
        set(MOLTENVK_ICD_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/MoltenVK_icd.json")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/macos/MoltenVK_icd.json.in"
            "${MOLTENVK_ICD_OUTPUT}"
            @ONLY
        )
    else()
        message(WARNING "MoltenVK dylib was not found; distributed macOS app may fail on machines without a local Vulkan SDK.")
    endif()

    enable_language(OBJCXX)

    file(GLOB_RECURSE PROJECT_OBJCXX_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm"
    )
    list(APPEND PROJECT_SOURCES ${PROJECT_OBJCXX_SOURCES})
endif()

set(MACOS_BUNDLE_INFO_PLIST_FILE "")
set(MACOS_BUNDLE_ICON_SOURCE "")
set(DUCK_DEPLOY_BUNDLE_SCRIPT "")
set(DUCK_BUILD_DMG_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build-dmg.sh")
if(APPLE)
    # إعداد ملف Info.plist وأيقونة التطبيق داخل حزمة macOS.
    set(MACOS_BUNDLE_ICON_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/assets/macos/DuckHunterStarter.icns")
    set(MACOS_BUNDLE_INFO_PLIST_FILE "${CMAKE_CURRENT_BINARY_DIR}/DuckHunterStarter-Info.plist")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in"
        "${MACOS_BUNDLE_INFO_PLIST_FILE}"
        @ONLY
    )
    string(JOIN ";" DUCK_BUNDLE_SEARCH_PATHS ${DUCK_BUNDLE_SEARCH_DIRS})
    add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_SOURCES})
else()
    add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
endif()

# إتاحة ترويسات المشروع المحلية لكل ملفات اللعبة.
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(${PROJECT_NAME} PRIVATE
    DUCK_ENABLE_VULKAN_VALIDATION=$<BOOL:${DUCK_ENABLE_VULKAN_VALIDATION}>
)

if(APPLE)
    # وضع الأيقونة داخل Resources وربط خصائص الحزمة الخاصة بتطبيقات macOS.
    if(EXISTS "${MACOS_BUNDLE_ICON_SOURCE}")
        set_source_files_properties("${MACOS_BUNDLE_ICON_SOURCE}" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )
        target_sources(${PROJECT_NAME} PRIVATE "${MACOS_BUNDLE_ICON_SOURCE}")
    endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${MACOS_BUNDLE_INFO_PLIST_FILE}"
        MACOSX_BUNDLE_BUNDLE_NAME "${MACOS_APP_DISPLAY_NAME}"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${MACOS_APP_BUNDLE_ID}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_ICON_FILE "${MACOS_APP_ICON_FILE}"
        BUILD_RPATH "${DUCK_BUNDLE_SEARCH_DIRS}"
        INSTALL_RPATH "@executable_path/../Frameworks"
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

    if(DUCK_MACOS_WANTS_UNIVERSAL)
        list(APPEND GLFW_MACOS_LIBRARY_SUFFIXES lib-universal lib-arm64 lib-x86_64)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
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
if(NOT (APPLE AND DUCK_MACOS_WANTS_UNIVERSAL))
    if(TARGET glfw)
        set(GLFW_TARGET glfw)
    elseif(TARGET glfw3)
        set(GLFW_TARGET glfw3)
    endif()
endif()

if(NOT GLFW_TARGET)
    set(GLFW_FALLBACK_LIBRARY "")
    set(GLFW_FALLBACK_INCLUDE_DIR "")

    if(APPLE)
        # بعض ملفات glfw3Config.cmake على جهاز التطوير تشير إلى lib-arm64 فقط.
        # عند بناء universal نتجاوز الـ imported target ونختار lib-universal يدويًا.
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

function(duck_get_imported_location target_name output_var)
    if(NOT TARGET "${target_name}")
        set(${output_var} "" PARENT_SCOPE)
        return()
    endif()

    get_target_property(_duck_is_imported "${target_name}" IMPORTED)
    if(NOT _duck_is_imported)
        set(${output_var} "" PARENT_SCOPE)
        return()
    endif()

    set(_duck_location "")
    if(CMAKE_BUILD_TYPE)
        string(TOUPPER "${CMAKE_BUILD_TYPE}" _duck_config)
        get_target_property(_duck_location "${target_name}" "IMPORTED_LOCATION_${_duck_config}")
    endif()

    if(NOT _duck_location)
        get_target_property(_duck_location "${target_name}" IMPORTED_LOCATION)
    endif()

    if(NOT _duck_location)
        get_target_property(_duck_imported_configs "${target_name}" IMPORTED_CONFIGURATIONS)
        foreach(_duck_config IN LISTS _duck_imported_configs)
            get_target_property(_duck_location "${target_name}" "IMPORTED_LOCATION_${_duck_config}")
            if(_duck_location)
                break()
            endif()
        endforeach()
    endif()

    set(${output_var} "${_duck_location}" PARENT_SCOPE)
endfunction()

function(duck_get_install_name_basename library_path output_var)
    if(NOT library_path OR NOT EXISTS "${library_path}")
        set(${output_var} "" PARENT_SCOPE)
        return()
    endif()

    execute_process(
        COMMAND /usr/bin/otool -D "${library_path}"
        RESULT_VARIABLE _duck_otool_result
        OUTPUT_VARIABLE _duck_otool_output
        ERROR_VARIABLE _duck_otool_error
    )
    if(NOT _duck_otool_result EQUAL 0)
        message(FATAL_ERROR
            "Failed to inspect install name for ${library_path}: ${_duck_otool_error}"
        )
    endif()

    string(REPLACE "\n" ";" _duck_otool_lines "${_duck_otool_output}")
    list(LENGTH _duck_otool_lines _duck_otool_length)
    if(_duck_otool_length LESS 2)
        set(${output_var} "" PARENT_SCOPE)
        return()
    endif()

    list(GET _duck_otool_lines 1 _duck_install_name)
    string(STRIP "${_duck_install_name}" _duck_install_name)
    get_filename_component(_duck_install_basename "${_duck_install_name}" NAME)
    set(${output_var} "${_duck_install_basename}" PARENT_SCOPE)
endfunction()

set(DUCK_GLFW_DYLIB_SOURCE "")
set(DUCK_GLFW_DYLIB_NAME "")
set(DUCK_VULKAN_DYLIB_SOURCE "")
set(DUCK_VULKAN_DYLIB_NAME "")
set(DUCK_VULKAN_REAL_DYLIB_SOURCE "")
set(DUCK_VULKAN_REAL_DYLIB_NAME "")
if(APPLE)
    duck_get_imported_location("${GLFW_TARGET}" DUCK_GLFW_IMPORTED_LOCATION)
    if(DUCK_GLFW_IMPORTED_LOCATION MATCHES "\\.dylib$")
        get_filename_component(DUCK_GLFW_DYLIB_SOURCE "${DUCK_GLFW_IMPORTED_LOCATION}" REALPATH)
        duck_get_install_name_basename("${DUCK_GLFW_IMPORTED_LOCATION}" DUCK_GLFW_DYLIB_NAME)
        if(NOT DUCK_GLFW_DYLIB_NAME)
            get_filename_component(DUCK_GLFW_DYLIB_NAME "${DUCK_GLFW_DYLIB_SOURCE}" NAME)
        endif()
    endif()

    duck_get_imported_location("Vulkan::Vulkan" DUCK_VULKAN_IMPORTED_LOCATION)
    if(DUCK_VULKAN_IMPORTED_LOCATION MATCHES "\\.dylib$")
        get_filename_component(DUCK_VULKAN_DYLIB_SOURCE "${DUCK_VULKAN_IMPORTED_LOCATION}" ABSOLUTE)
        get_filename_component(DUCK_VULKAN_REAL_DYLIB_SOURCE "${DUCK_VULKAN_IMPORTED_LOCATION}" REALPATH)
        get_filename_component(DUCK_VULKAN_REAL_DYLIB_NAME "${DUCK_VULKAN_REAL_DYLIB_SOURCE}" NAME)
        duck_get_install_name_basename("${DUCK_VULKAN_IMPORTED_LOCATION}" DUCK_VULKAN_DYLIB_NAME)
        if(NOT DUCK_VULKAN_DYLIB_NAME)
            get_filename_component(DUCK_VULKAN_DYLIB_NAME "${DUCK_VULKAN_DYLIB_SOURCE}" NAME)
        endif()
    endif()

    set(DUCK_DEPLOY_BUNDLE_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/DuckDeployBundle.cmake")
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/macos/DeployBundle.cmake.in"
        "${DUCK_DEPLOY_BUNDLE_SCRIPT}"
        @ONLY
    )
endif()

if(APPLE)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreFoundation"
            "-framework CoreGraphics"
            "-framework CoreText"
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
            COMMAND ${CMAKE_COMMAND} -E remove_directory
                "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/assets"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/assets"
                "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/assets"
            COMMENT "نسخ أصول اللعبة داخل حزمة تطبيق macOS"
        )
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Frameworks"
        COMMAND ${CMAKE_COMMAND} -E remove_directory
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/vulkan"
        COMMENT "تنظيف آثار النشر من حزمة التطوير"
    )

    # تثبيت الحزمة كبرنامج مستقل مع الأصول داخل Resources.
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )

    # بعد التثبيت ننسخ المكتبات الديناميكية المطلوبة إلى داخل الحزمة ونعدّل المسارات.
    install(CODE "
        get_filename_component(_duck_install_prefix \"\${CMAKE_INSTALL_PREFIX}\" ABSOLUTE)
        set(_duck_bundle_path \"\${_duck_install_prefix}/${PROJECT_NAME}.app\")
        execute_process(
            COMMAND \"${CMAKE_COMMAND}\"
                -DDUCK_BUNDLE_PATH=\${_duck_bundle_path}
                -P \"${DUCK_DEPLOY_BUNDLE_SCRIPT}\"
            COMMAND_ERROR_IS_FATAL ANY
        )
    ")
    add_custom_target(dist
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} --install "${CMAKE_BINARY_DIR}" --prefix "${CMAKE_BINARY_DIR}/dist"
        DEPENDS ${PROJECT_NAME}
        COMMENT "Build a distributable macOS app bundle in ${CMAKE_BINARY_DIR}/dist"
    )
    add_custom_target(dmg
        COMMAND "${DUCK_BUILD_DMG_SCRIPT}" "${CMAKE_BINARY_DIR}/dist" "${CMAKE_BINARY_DIR}"
        DEPENDS dist
        COMMENT "Build a distributable DMG from ${CMAKE_BINARY_DIR}/dist"
        VERBATIM
    )
else()
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            DESTINATION "${CMAKE_INSTALL_BINDIR}"
            PATTERN ".DS_Store" EXCLUDE
        )
    endif()
endif()
