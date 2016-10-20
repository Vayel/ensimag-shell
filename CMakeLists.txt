cmake_minimum_required(VERSION 2.6)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

project(Ensishell)
enable_testing()

#######
# Detect presence of minitest using gem
# install it if not available
#######
execute_process(COMMAND gem which minitest RESULT_VARIABLE GEM_MINITEST)
if (NOT ${GEM_MINITEST} EQUAL 0)
  execute_process(COMMAND gem install minitest RESULT_VARIABLE GEM_MINITEST_INSTALL)
  if (NOT ${GEM_MINITEST_INSTALL} EQUAL 0)
    message(FATAL_ERROR "Ruby: gem: minitest unavailable and uninstallable")
  endif ()
endif ()

#######
# Detect Guile en utilisant pkg-config
# Si guile est desactivé dans ensishell.c vous pouvez commenter les lignes
#####
# Guile 2.0 by default

execute_process(COMMAND pkg-config --cflags guile-2.0 OUTPUT_VARIABLE GUILE_PKG_CFLAGS)
execute_process(COMMAND pkg-config --libs guile-2.0 OUTPUT_VARIABLE GUILE_PKG_LDFLAGS)

# Guile 1.8 if guile 2.0 is not available
if ("${GUILE_PKG_LDFLAGS}" STREQUAL "")
  execute_process(COMMAND pkg-config --cflags guile-1.8 OUTPUT_VARIABLE GUILE_PKG_CFLAGS)
  execute_process(COMMAND pkg-config --libs guile-1.8 OUTPUT_VARIABLE GUILE_PKG_LDFLAGS)
endif()

# aucun Guile n'a été trouvé
if (NOT "${GUILE_PKG_LDFLAGS}" STREQUAL "")
  set(USE_GUILE 1)
  string(STRIP ${GUILE_PKG_CFLAGS} GUILE_CFLAGS)
  string(STRIP ${GUILE_PKG_LDFLAGS} GUILE_LDFLAGS)

  # Ensimag debug: if libltld.so is required but not there, put explicit path
  find_library(LIBLTDL ltdl)
  if (${GUILE_LDFLAGS} MATCHES "-lltdl" AND NOT ${LIBLTDL})
    string(REPLACE "-lltdl" "/usr/lib64/libltdl.so.7" GUILE_LDFLAGS ${GUILE_LDFLAGS})
  endif()
else()
  set(USE_GUILE 0)
endif()

####
# Detect if gnu readline header is present, otherwise use internal readline
####
find_path(READLINE_PATH "include/readline/readline.h")
if (NOT "${READLINE_PATH}" STREQUAL "")
  set(USE_GNU_READLINE 1)
  set(READLINE_LDFLAGS readline history)
else()
  set(USE_GNU_READLINE 0)
  set(READLINE_LDFLAGS)
endif()


# Debug build

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${GUILE_CFLAGS} -Wall -Werror -std=gnu99")

#########
# Gestion des variantes
#########
# Vous devez editer les lignes suivantes pour y mettre vos logins
# et le numéro de la variante que vous avez choisi
#########
set(VARIANTE_LOGINS lefoulov moucheta)
set(VARIANTE_SUJET 7)
###
list(SORT VARIANTE_LOGINS)
if (VARIANTE_LOGINS MATCHES "login[123]")
  message(FATAL_ERROR "** ERREUR **: Vous devez modifier CMakeLists.txt pour y mettre vos logins")
endif()
if (VARIANTE_SUJET EQUAL -1)
  message(FATAL_ERROR "** ERREUR **: Vous devez modifier CMakeLists.txt pour y mettre le numéro de votre variante du sujet")
endif()

configure_file (
  src/variante.h.in
  ${CMAKE_SOURCE_DIR}/src/variante.h
)

#########
# Fin de gestion des variantes
#########

##
# Si vous utilisez plusieurs fichiers, en plus de ensishell.c, pour votre
# shell il faut les ajouter ici
##
add_executable(ensishell src/readcmd.c src/ensishell.c)
target_link_libraries(ensishell ${READLINE_LDFLAGS} ${GUILE_LDFLAGS})

##
# Programme de test
##
add_test(UnitShellTests ../tests/allShellTests.rb)

##
# Ajout d'une cible pour lancer les tests de manière verbeuse
##
add_custom_target(check ../tests/allShellTests.rb)

##
# Construction de l'archive
##
string(REPLACE ";" "-" LOGINS_SANS_POINTVIRGULE "${VARIANTE_LOGINS}")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH ${LOGINS_SANS_POINTVIRGULE})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
  "~$"
  "\\\\.o$"
  "^${PROJECT_SOURCE_DIR}/build/"
  "^${PROJECT_SOURCE_DIR}/.git/"
)
include(CPack)
