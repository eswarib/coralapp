cmake_minimum_required(VERSION 3.20)
project(coral LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Allow using vcpkg toolchain on Windows
# cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows

if(UNIX)
	# Collect sources (exclude tests)
	file(GLOB_RECURSE CORAL_SOURCES
		"src/*.cpp"
	)
	# Remove test sources if any matched
	list(FILTER CORAL_SOURCES EXCLUDE REGEX "/test/.*\\.cpp$")

	add_executable(coral ${CORAL_SOURCES})

	# pkg-config for glib/gio and other libs
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GLIB2 REQUIRED glib-2.0)
	pkg_check_modules(GIO2 REQUIRED gio-2.0)

	# Optional: portal, xkbcommon
	pkg_check_modules(PORTAL libportal QUIET)
	pkg_check_modules(XKBCOMMON xkbcommon QUIET)

	# Audio libs via pkg-config
	pkg_check_modules(PORTAUDIO portaudio-2.0 QUIET)
	if(NOT PORTAUDIO_FOUND)
		# Fallback name
		pkg_check_modules(PORTAUDIO portaudio QUIET)
	endif()
	pkg_check_modules(SNDFILE sndfile REQUIRED)

	target_include_directories(coral PRIVATE
		${GLIB2_INCLUDE_DIRS}
		${GIO2_INCLUDE_DIRS}
		${PORTAL_INCLUDE_DIRS}
		${XKBCOMMON_INCLUDE_DIRS}
		${PORTAUDIO_INCLUDE_DIRS}
		${SNDFILE_INCLUDE_DIRS}
	)

	target_link_directories(coral PRIVATE
		${GLIB2_LIBRARY_DIRS}
		${GIO2_LIBRARY_DIRS}
		${PORTAL_LIBRARY_DIRS}
		${XKBCOMMON_LIBRARY_DIRS}
		${PORTAUDIO_LIBRARY_DIRS}
		${SNDFILE_LIBRARY_DIRS}
	)

	target_compile_options(coral PRIVATE
		${GLIB2_CFLAGS_OTHER}
		${GIO2_CFLAGS_OTHER}
		${PORTAL_CFLAGS_OTHER}
		${XKBCOMMON_CFLAGS_OTHER}
		${PORTAUDIO_CFLAGS_OTHER}
		${SNDFILE_CFLAGS_OTHER}
	)

	# Link Linux libraries
	target_link_libraries(coral PRIVATE
		${GLIB2_LIBRARIES}
		${GIO2_LIBRARIES}
		${PORTAL_LIBRARIES}
		${XKBCOMMON_LIBRARIES}
		${PORTAUDIO_LIBRARIES}
		${SNDFILE_LIBRARIES}
		X11
		Xtst
		Xinerama
	)

	# Link libxdo if available as static in repo; else system -lxdo
	set(LIBXDO_STATIC "${CMAKE_CURRENT_LIST_DIR}/../libxdo/libxdo.a")
	if(EXISTS "${LIBXDO_STATIC}")
		target_link_libraries(coral PRIVATE "${LIBXDO_STATIC}")
	else()
		find_library(LIBXDO xdo)
		if(LIBXDO)
			target_link_libraries(coral PRIVATE ${LIBXDO})
		endif()
	endif()

	# RPATH to local lib folder if used
	set_target_properties(coral PROPERTIES
		BUILD_RPATH "${CMAKE_CURRENT_LIST_DIR}/lib"
		INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
	)

	# Install rules (portable layout)
	install(TARGETS coral RUNTIME DESTINATION .)

else() # WIN32
    # Collect sources and exclude Linux-only files
    file(GLOB_RECURSE CORAL_SOURCES
        "src/*.cpp"
    )
    # Exclude X11/Wayland/AT-SPI sources from Windows build
    list(FILTER CORAL_SOURCES EXCLUDE REGEX "/src/X11.*\\.cpp$")
    list(FILTER CORAL_SOURCES EXCLUDE REGEX "/src/Wayland.*\\.cpp$")
    list(FILTER CORAL_SOURCES EXCLUDE REGEX "/src/A11yInjector\\.cpp$")

    add_executable(coral ${CORAL_SOURCES})

    # Dependencies via vcpkg recommended:
    #   vcpkg install portaudio:x64-windows libsndfile:x64-windows
    find_package(PortAudio CONFIG REQUIRED)
    find_package(sndfile CONFIG REQUIRED)

    # Optional: Whisper library on Windows (user-provided)
    set(WHISPER_HINT "${CMAKE_CURRENT_LIST_DIR}/lib")
    find_library(WHISPER_LIBRARY NAMES whisper whisper.dll.lib HINTS ${WHISPER_HINT})
    if(WHISPER_LIBRARY)
        target_link_libraries(coral PRIVATE ${WHISPER_LIBRARY})
        target_include_directories(coral PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/../whispercpp/include
            ${CMAKE_CURRENT_LIST_DIR}/../whispercpp/ggml/include
        )
    else()
        message(WARNING "Whisper library not found. Ensure whisper(.lib) is available and set in coral/lib or provide -DWHISPER_LIBRARY=...")
    endif()

    target_link_libraries(coral PRIVATE
        PortAudio::PortAudio
        sndfile::sndfile
        user32 # For SendInput in WindowsInjector
    )

    # Install rules (portable layout)
    install(TARGETS coral RUNTIME DESTINATION .)
endif()

# CPack (Windows NSIS) – safe to include always; only generates when packaging
set(CPACK_PACKAGE_NAME "Coral")
set(CPACK_PACKAGE_VENDOR "Coral Project")
set(CPACK_PACKAGE_VERSION "0.2.0")
set(CPACK_PACKAGE_CONTACT "")
if(WIN32)
	set(CPACK_GENERATOR NSIS)
	set(CPACK_NSIS_DISPLAY_NAME "Coral")
	set(CPACK_NSIS_PACKAGE_NAME "Coral")
	set(CPACK_NSIS_CONTACT "")
	set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_LIST_DIR}/logo/coral.ico")
endif()
include(CPack) 