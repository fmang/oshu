configure_file(oshu.1.in oshu.1)
configure_file(oshu-library.1.in oshu-library.1)

install(
	FILES "${CMAKE_CURRENT_BINARY_DIR}/oshu.1"
	DESTINATION "${CMAKE_INSTALL_MANDIR}/man1"
)
