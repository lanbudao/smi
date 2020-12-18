cmake_minimum_required(VERSION 2.8.4)

project(ftgl)
set(PROJECT_VERSION ${SMI_MAJOR}.${SMI_MINOR}.${SMI_PATCH})

set(MODULE ftgl)

# Use C++11
#if(CMAKE_COMPILER_IS_GNUCXX)
#    set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
#    message(STATUS "optional:-std=c++11")
#endif(CMAKE_COMPILER_IS_GNUCXX)

list(APPEND EXTRA_DEFS -DSMI_HAVE_FTGL=1 -D__STDC_CONSTANT_MACROS)
LIST(APPEND EXTRA_DEFS -DFTGL_LIBRARY)

# Check freetype modules =0976
if (EXISTS ${FREETYPE_DIR})
    message("FTGL-HAVE-FREETYPE: " ${FREETYPE_DIR})
    list(APPEND INCLUDE_DIR ${FREETYPE_DIR}/include/freetype2)
    find_library(LIB_FREETYPE freetype ${FREETYPE_DIR}/lib)
    list(APPEND EXTRA_LIBS ${FREETYPE_DIR})
endif()

list(APPEND SOURCES
	FTBuffer.cpp
	FTCharmap.cpp
	FTContour.cpp
	FTFace.cpp
	FTFont/FTBitmapFont.cpp
	FTFont/FTBufferFont.cpp
	FTFont/FTExtrudeFont.cpp
	FTFont/FTFont.cpp
	FTFont/FTFontGlue.cpp
	FTFont/FTOutlineFont.cpp
	FTFont/FTPixmapFont.cpp
	FTFont/FTPolygonFont.cpp
	FTFont/FTTextureFont.cpp
	FTGlyph/FTBitmapGlyph.cpp
	FTGlyph/FTBufferGlyph.cpp
	FTGlyph/FTExtrudeGlyph.cpp
	FTGlyph/FTGlyph.cpp
	FTGlyph/FTGlyphGlue.cpp
	FTGlyph/FTOutlineGlyph.cpp
	FTGlyph/FTPixmapGlyph.cpp
	FTGlyph/FTPolygonGlyph.cpp
	FTGlyph/FTTextureGlyph.cpp
	FTGlyphContainer.cpp
	FTLayout/FTLayout.cpp
	FTLayout/FTLayoutGlue.cpp
	FTLayout/FTSimpleLayout.cpp
	FTLibrary.cpp
	FTPoint.cpp
	FTSize.cpp
	FTVectoriser.cpp)

list(APPEND HEADERS
	config.h
    FTCharToGlyphIndexMap.h
    FTCharmap.h
    FTContour.h
    FTFace.h
    FTFont/FTBitmapFontImpl.h
    FTFont/FTBufferFontImpl.h
    FTFont/FTExtrudeFontImpl.h
    FTFont/FTFontImpl.h
    FTFont/FTOutlineFontImpl.h
    FTFont/FTPixmapFontImpl.h
    FTFont/FTPolygonFontImpl.h
    FTFont/FTTextureFontImpl.h
    FTGL/FTBBox.h
    FTGL/FTBitmapGlyph.h
    FTGL/FTBuffer.h
    FTGL/FTBufferFont.h
    FTGL/FTBufferGlyph.h
    FTGL/FTExtrdGlyph.h
    FTGL/FTFont.h
    FTGL/FTGLBitmapFont.h
    FTGL/FTGLExtrdFont.h
    FTGL/FTGLOutlineFont.h
    FTGL/FTGLPixmapFont.h
    FTGL/FTGLPolygonFont.h
    FTGL/FTGLTextureFont.h
    FTGL/FTGlyph.h
    FTGL/FTLayout.h
    FTGL/FTOutlineGlyph.h
    FTGL/FTPixmapGlyph.h
    FTGL/FTPoint.h
    FTGL/FTPolyGlyph.h
    FTGL/FTSimpleLayout.h
    FTGL/FTTextureGlyph.h
    FTGL/ftgl.h
    FTGlyph/FTBitmapGlyphImpl.h
    FTGlyph/FTBufferGlyphImpl.h
    FTGlyph/FTExtrudeGlyphImpl.h
    FTGlyph/FTGlyphImpl.h
    FTGlyph/FTOutlineGlyphImpl.h
    FTGlyph/FTPixmapGlyphImpl.h
    FTGlyph/FTPolygonGlyphImpl.h
    FTGlyph/FTTextureGlyphImpl.h
    FTGlyphContainer.h
    FTInternals.h
    FTLayout/FTLayoutImpl.h
    FTLayout/FTSimpleLayoutImpl.h
    FTLibrary.h
    FTList.h
    FTSize.h
    FTUnicode.h
    FTVector.h
    FTVectoriser.h)
	
# include path
list(APPEND INCLUDE_DIR .
        FTFont
		FTGL
		FTGlyph
		FTLayout)
list(APPEND INCLUDE_DIR ${EXTRA_INCLUDE})		
include_directories(${INCLUDE_DIR})
		
# Output path
set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin/${CMAKE_INTDIR})
add_library(${MODULE} SHARED ${SOURCES} ${HEADERS})

target_compile_definitions(${MODULE} PRIVATE ${EXTRA_DEFS})
message(LIB_FREETYPE: ${LIB_FREETYPE})
target_link_libraries(${MODULE} ${LIB_FREETYPE} opengl32 glu32)

set_target_properties(${MODULE} PROPERTIES
        VERSION 1.0
        SOVERSION 1.0
        OUTPUT_NAME ftgl)
