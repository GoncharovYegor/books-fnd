include_directories(${CMAKE_CURRENT_LIST_DIR})

set(SEVENZIP_SRC_DIR "${EXT_ROOT}/zip/7z/7-zip/CPP/7zip/Bundles/Format7zF")
set(SEVENZIP_BUILD_DIR "${CMAKE_BINARY_DIR}/7z-build")

if(WIN32)
	set(7zip_BIN_FILENAME 7z.dll)
	set(7zip_BIN_PATH ${CMAKE_BINARY_DIR}/bin)
    set(SEVENZIP_OUTPUT "${SEVENZIP_BUILD_DIR}/${7zip_BIN_FILENAME}")
    file(TO_NATIVE_PATH "${SEVENZIP_BUILD_DIR}" SEVENZIP_BUILD_DIR_NATIVE)
	install(FILES "${SEVENZIP_BUILD_DIR}/${7zip_BIN_FILENAME}" DESTINATION .)

    add_custom_command(
        OUTPUT "${SEVENZIP_OUTPUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SEVENZIP_BUILD_DIR}"
        COMMAND nmake /NOLOGO PLATFORM=x64 NEW_COMPILER=1 "O=${SEVENZIP_BUILD_DIR_NATIVE}"
        WORKING_DIRECTORY "${SEVENZIP_SRC_DIR}"
        COMMENT "Building 7z.dll with nmake..."
        VERBATIM
    )
elseif(UNIX)
	set(7zip_BIN_FILENAME 7z.so)
	set(7zip_BIN_PATH ${CMAKE_BINARY_DIR}/lib)
	set(SEVENZIP_OUTPUT "${SEVENZIP_BUILD_DIR}/${7zip_BIN_FILENAME}")
	install(FILES "${SEVENZIP_BUILD_DIR}/${7zip_BIN_FILENAME}" DESTINATION lib)
	
    set(SEVENZIP_CC "${CMAKE_C_COMPILER}")
    set(SEVENZIP_CXX "${CMAKE_CXX_COMPILER}")

    if(APPLE)
    	if(CMAKE_OSX_ARCHITECTURES)
	        set(TARGET_ARCH "${CMAKE_OSX_ARCHITECTURES}")
    	else()
        	set(TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
	    endif()

	    if(TARGET_ARCH STREQUAL "arm64")
    	    set(SEVENZIP_MAK "../../cmpl_mac_arm64.mak")
	    elseif(TARGET_ARCH STREQUAL "x86_64")
    	    set(SEVENZIP_MAK "../../cmpl_mac_x64.mak")
	    else()
    	    message(FATAL_ERROR "Unsupported macOS architecture for 7-Zip: ${TARGET_ARCH}")
	    endif()
    else()
        set(SEVENZIP_MAK "../../cmpl_gcc.mak")
        
        include(CheckCXXSourceCompiles)
	check_c_source_compiles("
    __attribute__((__target__(\"sha512,avx2\")))
    static void test_sha512(void) {
        __asm__ volatile(\"vsha512rnds2 %%xmm1, %%ymm7, %%ymm0\" ::: \"ymm0\");
    }
    int main() {
        test_sha512();
        return 0;
    }
" COMPILER_SUPPORTS_SHA512)
	
	if(NOT COMPILER_SUPPORTS_SHA512)
    		find_program(GCC12_C_COMPILER gcc-12)
    		if(GCC12_C_COMPILER)
        		message(STATUS "Assembler lacks SHA-512 support; falling back to gcc-12 for 7z")
        		set(SEVENZIP_CC "${GCC12_C_COMPILER}")
        	else()
        		message(FATAL_ERROR "GCC-12 or Assembler with SHA-512 support required")
		endif()
	endif()
    endif()

    include(ProcessorCount)
    ProcessorCount(NPROC)
    if(NPROC EQUAL 0)
        set(NPROC 4)
    endif()

    add_custom_command(
        OUTPUT "${SEVENZIP_OUTPUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SEVENZIP_BUILD_DIR}"
        COMMAND make -j${NPROC} -f "${SEVENZIP_MAK}"
        	"O=${SEVENZIP_BUILD_DIR}"
        	"CC=${SEVENZIP_CC}"
        	"CXX=${SEVENZIP_CXX}"
        	"CFLAGS_WARN_WALL=-Wall -Wextra"
        WORKING_DIRECTORY "${SEVENZIP_SRC_DIR}"
        COMMENT "Building 7z.so with make..."
        VERBATIM
    )
else()
	message(FATAL_ERROR "Unsupported host system: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

set(SEVENZIP_FINAL_FILE ${7zip_BIN_PATH}/${7zip_BIN_FILENAME})

add_custom_command(
    OUTPUT "${SEVENZIP_FINAL_FILE}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SEVENZIP_OUTPUT}" "${SEVENZIP_FINAL_FILE}"
    DEPENDS "${SEVENZIP_OUTPUT}"
    COMMENT "Copying ${7zip_BIN_FILENAME} to ${7zip_BIN_PATH}..."
    VERBATIM
)

add_custom_target(7z ALL DEPENDS "${SEVENZIP_FINAL_FILE}")
if(MSVC)
	set_property(TARGET 7z PROPERTY FOLDER Foundation/Zip)
endif()

set(BIT7Z_AUTO_FORMAT ON CACHE BOOL "enable auto format support" FORCE)
set(BIT7Z_USE_NATIVE_STRING ON CACHE BOOL "enable native string" FORCE)
set(BIT7Z_USE_STD_BYTE ON CACHE BOOL "enable std::byte" FORCE)
set(BIT7Z_CUSTOM_7ZIP_PATH "${EXT_ROOT}/zip/7z/7-zip" CACHE PATH "")
if(APPLE)
	set(BIT7Z_USE_LEGACY_IUNKNOWN ON CACHE BOOL "enable p7zip-compatible IUnknown" FORCE)
endif()
add_subdirectory(${EXT_ROOT}/zip/7z/bit7z)
if(MSVC)
	get_target_property(BIT7Z_LIBRARY_TARGET bit7z ALIASED_TARGET)
	if(NOT BIT7Z_LIBRARY_TARGET)
		set(BIT7Z_LIBRARY_TARGET bit7z)
	endif()
	set_property(TARGET ${BIT7Z_LIBRARY_TARGET} PROPERTY FOLDER ThirdParty)
endif()

AddTarget(zip	shared_lib
	PROJECT_GROUP Foundation/Zip
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}"
	EXCLUDE_SOURCES
		${CMAKE_CURRENT_LIST_DIR}/unit-tests
    INCLUDE_DIRECTORIES
        "${EXT_ROOT}/zip/bit7z/include"
	LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Core
        bit7z
	LINK_TARGETS
		fnd
		logging
		platform
	DEPENDENCIES
		7z
)

AddGTest(zip
    MOCK
	PROJECT_GROUP UT/Foundation/Zip
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}/unit-tests"
    LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Core
    LINK_TARGETS
		zip
)
