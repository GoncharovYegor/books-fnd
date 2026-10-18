AddTarget(util	shared_lib
	PROJECT_GROUP Foundation
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}"
	COMPILE_DEFINITIONS
		cimg_display=0
	INCLUDE_DIRECTORIES
		"${EXT_ROOT}/libmobi/src"
	LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::Gui
		XercesC::XercesC
		cimg::cimg
		[ GNU atomic ]
	LINK_TARGETS
		fljxl
		logging
		mobi
		platform
		zip
	DEPENDENCIES
		flicu
)
