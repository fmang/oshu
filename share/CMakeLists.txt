set(DEFAULT_SKIN minimal CACHE STRING "Default skin")
set(SKINS ${DEFAULT_SKIN} ${SKINS} CACHE STRING ";-list of enabled skins" FORCE)

add_custom_command(
	OUTPUT skins/default
	COMMAND ${CMAKE_COMMAND} -E create_symlink ${DEFAULT_SKIN} skins/default
	COMMENT "Default skin set to ${DEFAULT_SKIN}"
	VERBATIM
)

add_custom_target(skins ALL DEPENDS skins/default)

install(
	DIRECTORY skins
	DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
)

install(
	DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/skins/default
	DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/skins
)

if (osu IN_LIST SKINS)
	file(
		DOWNLOAD https://www.mg0.fr/oshu/skins/osu-v1.tar.gz ${CMAKE_CURRENT_BINARY_DIR}/skins/osu.tar.gz
		EXPECTED_HASH SHA256=bc645534371bf5ccba66d4a6729dc1deffb6f148b6ce115efd305f9ffab1d183
	)
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/skins/osu/README.md
		COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_CURRENT_BINARY_DIR}/skins/osu.tar.gz
		COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_SOURCE_DIR}/skins/osu/README.md
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/skins
		DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/skins/osu.tar.gz
		COMMENT "Unpacking the osu skin"
		VERBATIM
	)
	add_custom_target(osu-skin DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/skins/osu/README.md)
	add_dependencies(skins osu-skin)
endif (osu IN_LIST SKINS)

add_subdirectory(desktop)
add_subdirectory(man)
