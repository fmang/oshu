# TODO
# The samples archive is downloaded even when the output already exists!

file(
	DOWNLOAD https://www.mg0.fr/oshu/samples-v1.tar.bz2 ${CMAKE_CURRENT_BINARY_DIR}/samples-v1.tar.bz2
	EXPECTED_HASH SHA256=3a7c47d0be9e820d2384adb77a0edc3ddd22b9a0689efe9c090f69246e0ea623
)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/samples/README.md
	COMMAND ${CMAKE_COMMAND} -E tar xjf ${CMAKE_CURRENT_BINARY_DIR}/samples-v1.tar.bz2
	COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_SOURCE_DIR}/samples/README.md
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/samples-v1.tar.bz2
	COMMENT "Unpacking the samples"
	VERBATIM
)

add_custom_target(samples ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/samples/README.md)

install(
	DIRECTORY samples
	DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
)
