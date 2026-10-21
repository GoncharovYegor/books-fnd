AddTarget(dbsqlite	shared_lib
	PROJECT_GROUP Foundation/Database/Impl
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}"
	INCLUDE_DIRECTORIES
		"${CMAKE_CURRENT_LIST_DIR}/../../../../sqlite/sqlite3pp/src"
		"${CMAKE_CURRENT_LIST_DIR}/../../../../sqlite/sqlite"
		"${CMAKE_CURRENT_LIST_DIR}/../../interface"
	LINK_LIBRARIES
		plog::plog
		Qt${QT_MAJOR_VERSION}::Core
	LINK_TARGETS
		logging
		sqlite3pp
		SQLite::SQLite3
	COMPILE_DEFINITIONS
		_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
)
