set(OSHU_DEFAULT_SKIN minimal CACHE STRING "Default skin")
set(OSHU_SKINS ${OSHU_DEFAULT_SKIN} CACHE STRING ";-list of enabled skins")
set(OSHU_ENABLED_SKINS ${OSHU_DEFAULT_SKIN} ${OSHU_SKINS})

add_custom_command(
	OUTPUT default
	COMMAND "${CMAKE_COMMAND}" -E create_symlink "${OSHU_DEFAULT_SKIN}" default
	COMMENT "Default skin set to ${OSHU_DEFAULT_SKIN}"
	VERBATIM
)

add_custom_target(skins ALL DEPENDS default)

install(
	DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/default"
	DESTINATION "${OSHU_SKINS_DIRECTORY}"
)

if ("minimal" IN_LIST OSHU_ENABLED_SKINS)
	install(
		DIRECTORY minimal
		DESTINATION "${OSHU_SKINS_DIRECTORY}"
	)
endif ("minimal" IN_LIST OSHU_ENABLED_SKINS)

if ("osu" IN_LIST OSHU_ENABLED_SKINS)
	file(
		DOWNLOAD "https://www.mg0.fr/oshu/skins/osu-v1.tar.gz" "${CMAKE_CURRENT_BINARY_DIR}/osu.tar.gz"
		EXPECTED_HASH SHA256=bc645534371bf5ccba66d4a6729dc1deffb6f148b6ce115efd305f9ffab1d183
	)
	add_custom_command(
		OUTPUT osu/README.md
		COMMAND "${CMAKE_COMMAND}" -E tar xf osu.tar.gz
		COMMAND "${CMAKE_COMMAND}" -E touch osu/README.md
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/osu.tar.gz"
		COMMENT "Unpacking the osu skin"
		VERBATIM
	)
	add_custom_target(osu-skin DEPENDS osu/README.md)
	add_dependencies(skins osu-skin)
	install(
		DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/osu"
		DESTINATION "${OSHU_SKINS_DIRECTORY}"
	)
endif ("osu" IN_LIST OSHU_ENABLED_SKINS)
