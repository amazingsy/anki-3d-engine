file(GLOB_RECURSE EDITOR_SOURCES *.cpp)
file(GLOB_RECURSE EDITOR_HEADERS *.h)

include_directories("..")

add_executable(Editor ${EDITOR_SOURCES} ${EDITOR_SOURCES})
target_compile_definitions(Editor PRIVATE -DANKI_SOURCE_FILE)
target_link_libraries(Editor AnKi)

installExecutable(Editor)