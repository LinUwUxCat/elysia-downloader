cmake_minimum_required(VERSION 3.16)
project(ElysiaDownloader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(CAIRO REQUIRED cairo)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GIO REQUIRED gio-2.0)

# Find libcurl for HTTP requests
find_package(CURL REQUIRED)

# Include directories
include_directories(${GTK4_INCLUDE_DIRS})
include_directories(${CAIRO_INCLUDE_DIRS})
include_directories(${GLIB_INCLUDE_DIRS})
include_directories(${GIO_INCLUDE_DIRS})

# Link directories
link_directories(${GTK4_LIBRARY_DIRS})
link_directories(${CAIRO_LIBRARY_DIRS})
link_directories(${GLIB_LIBRARY_DIRS})
link_directories(${GIO_LIBRARY_DIRS})

# Add executable
add_executable(ElysiaDownloader 
    src/main.cpp
    src/main_window.cpp
    src/danbooru_client.cpp
    src/image_downloader.cpp
)

# Link libraries
target_link_libraries(ElysiaDownloader 
    ${GTK4_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${GIO_LIBRARIES}
    CURL::libcurl
)

# Compiler flags
target_compile_options(ElysiaDownloader PRIVATE ${GTK4_CFLAGS_OTHER})
target_compile_options(ElysiaDownloader PRIVATE ${CAIRO_CFLAGS_OTHER})
target_compile_options(ElysiaDownloader PRIVATE ${GLIB_CFLAGS_OTHER})
target_compile_options(ElysiaDownloader PRIVATE ${GIO_CFLAGS_OTHER})

# Install targets
install(TARGETS ElysiaDownloader DESTINATION bin)


# Create symbolic links for different icon sizes
install(CODE "
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink 
        ../../ely-downloader.png 
        \$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/share/icons/hicolor/scalable/apps/ely-downloader.png)
")


