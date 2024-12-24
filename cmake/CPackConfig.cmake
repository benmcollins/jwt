# This file will be configured to contain variables for CPack. These variables
# should be set in the CMake list file of the project before CPack module is
# included. The list of available CPACK_xxx variables and their associated
# documentation may be obtained using
#  cpack --help-variable-list
#
# Some variables are common to all generators (e.g. CPACK_PACKAGE_NAME)
# and some are specific to a generator
# (e.g. CPACK_NSIS_EXTRA_INSTALL_COMMANDS). The generator specific variables
# usually begin with CPACK_<GENNAME>_xxxx.

set(CPACK_BINARY_TBZ2 "OFF")
set(CPACK_GENERATOR "TBZ2")
set(CPACK_PACKAGE_NAME "libjwt")
set(CPACK_PACKAGE_VENDOR "maClara, LLC")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_TBZ2 "ON")
set(CPACK_IGNORE_FILES "/\\.git/" "\\.gitignore" "\\.swp\$" "\\.DS_Store" "\\.travis.yml")
list(APPEND CPACK_IGNORE_FILES "/build/" "Makefile.*" "configure.ac" "/m4/" "Doxygen\\.mk")
list(APPEND CPACK_IGNORE_FILES "/dist/" "jwt_export\\.h\\.in" "/m4-local/")
string(TOLOWER ${CPACK_PACKAGE_NAME} CPACK_PACKAGE_NAME)
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
