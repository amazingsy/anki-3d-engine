FILE(GLOB_RECURSE TXML2_SOURCES *.cpp)
FILE(GLOB_RECURSE TXML2_HEADERS *.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

ADD_LIBRARY(AnKiTinyXml2 ${TXML2_SOURCES} ${TXML2_HEADERS})
