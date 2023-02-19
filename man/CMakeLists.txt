# Define the path to your input file
set(INPUT_FILE "zoslib.1")

# Define the output directories for the ASCII and man page files
set(ASCII_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(MAN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# Create the output directories
file(MAKE_DIRECTORY "${ASCII_OUTPUT_DIR}")
file(MAKE_DIRECTORY "${MAN_OUTPUT_DIR}")

# Generate the ASCII formatted text file
add_custom_command(
  OUTPUT "${ASCII_OUTPUT_DIR}/${INPUT_FILE}.txt"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}"
  COMMAND groff -man -Tascii "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}" > "${ASCII_OUTPUT_DIR}/${INPUT_FILE}.txt"
)

# Generate the man page
add_custom_command(
  OUTPUT "${MAN_OUTPUT_DIR}/${INPUT_FILE}"
  DEPENDS "${ASCII_OUTPUT_DIR}/${INPUT_FILE}.txt"
  COMMAND nroff -man "${ASCII_OUTPUT_DIR}/${INPUT_FILE}.txt" > "${MAN_OUTPUT_DIR}/${INPUT_FILE}"
)

# Define the install rule for the man page
install(FILES "${MAN_OUTPUT_DIR}/${INPUT_FILE}" DESTINATION "${CMAKE_INSTALL_PREFIX}/share/man/man1")

# Add the man page to the list of targets to be installed
#install(TARGETS libzoslib DESTINATION "${CMAKE_INSTALL_PREFIX}")

# Add the dependencies to ensure that the man page is generated before installation
#add_dependencies(libzoslib "${MAN_OUTPUT_DIR}/${INPUT_FILE}")
