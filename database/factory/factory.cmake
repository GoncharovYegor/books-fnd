AddTarget(dbfactory	shared_lib
	PROJECT_GROUP Foundation/Database
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}"
	LINK_LIBRARIES
		Boost::headers
		Qt${QT_MAJOR_VERSION}::Core
	LINK_TARGETS
		dbsqlite
		logging
)
