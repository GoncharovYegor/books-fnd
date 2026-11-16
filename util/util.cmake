option(USE_LIBXML2 ON)
add_subdirectory(${EXT_ROOT}/libmobi)
if(MSVC)
	set_property(TARGET mobi PROPERTY FOLDER ThirdParty)
	set_target_properties(mobi PROPERTIES COMPILE_OPTIONS /W0)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_C_COMPILER_ID STREQUAL "GNU")
	target_compile_options(mobi PRIVATE "$<$<COMPILE_LANGUAGE:C>:-Wno-stringop-overread>")
endif()

AddTarget(util	shared_lib
	PROJECT_GROUP Foundation
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}"
	COMPILE_DEFINITIONS
		cimg_display=0
	INCLUDE_DIRECTORIES
		"${EXT_ROOT}/libmobi/src"
	EXCLUDE_SOURCES
		${CMAKE_CURRENT_LIST_DIR}/unit-tests
	LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::Gui
		XercesC::XercesC
		cimg::cimg
		[ GCC atomic ]
	LINK_TARGETS
		fljxl
		logging
		mobi
		platform
		zip
	DEPENDENCIES
		flicu
)

AddGTest(util
	MOCK
	PROJECT_GROUP UT/Foundation
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}/unit-tests"
	LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Gui
	LINK_TARGETS
		util
)
