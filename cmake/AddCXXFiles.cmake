function(add_cxx_files TARGET)
	file(GLOB_RECURSE JIP_INCLUDE_FILES
	LIST_DIRECTORIES false
	CONFIGURE_DEPENDS
	"external/JIP-LN-NVSE/internal/*.h"
	"external/JIP-LN-NVSE/nvse/*.h"
	)

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/external/JIP-LN-NVSE
		PREFIX "JIP-LN Header Files"
		FILES ${JIP_INCLUDE_FILES})

	target_sources("${TARGET}" PUBLIC ${JIP_INCLUDE_FILES})

	file(GLOB_RECURSE JIP_SOURCE_FILES
	LIST_DIRECTORIES false
	CONFIGURE_DEPENDS
	"external/JIP-LN-NVSE/internal/*.cpp"
	"external/JIP-LN-NVSE/nvse/*.cpp"
	)

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/external/JIP-LN-NVSE
		PREFIX "JIP-LN Source Files"
		FILES ${JIP_SOURCE_FILES})

	target_sources("${TARGET}" PUBLIC ${JIP_SOURCE_FILES})

	file(GLOB_RECURSE INCLUDE_FILES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"include/*.h"
		"include/*.hpp"
		"include/*.hxx"
		"include/*.inl"
		"include/*.cpp"
	)

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include
		PREFIX "Header Files"
		FILES ${INCLUDE_FILES})

	target_sources("${TARGET}" PUBLIC ${INCLUDE_FILES})

	file(GLOB_RECURSE HEADER_FILES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"src/*.h"
		"src/*.hpp"
		"src/*.hxx"
		"src/*.inl"
	)

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
		PREFIX "Header Files"
		FILES ${HEADER_FILES})

	target_sources("${TARGET}" PRIVATE ${HEADER_FILES})

	file(GLOB_RECURSE SOURCE_FILES
		LIST_DIRECTORIES false
		CONFIGURE_DEPENDS
		"src/*.cpp"
		"src/*.cxx"
	)

	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
		PREFIX "Source Files"
		FILES ${SOURCE_FILES})

	target_sources("${TARGET}" PRIVATE ${SOURCE_FILES})

endfunction()
