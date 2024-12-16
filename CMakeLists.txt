cmake_minimum_required(VERSION 3.25.1)
project(TheMind C)

set(CMAKE_C_STANDARD 17)

add_executable(TheMindServeur src/main.c
        src/playersRessources.c
        src/Game.c
        src/queue.c
        src/utils.c
        src/statsManager.c)
add_executable(TheMindRobot TheMindRobot/src/robot.c
        TheMindRobot/src/GameState.c
        TheMindRobot/src/parser.c
        src/queue.c
)
add_executable(TheMindClient TheMindClient/src/main.c
        TheMindClient/src/utils.c
)

target_sources(TheMindServeur PRIVATE
        src/playersRessources.h
        src/Game.h
        src/queue.h
        src/utils.h
        src/statsManager.h
        src/ANSI-color-codes.h
)
target_sources(TheMindClient PRIVATE
        TheMindClient/src/utils.h
        src/ANSI-color-codes.h
)
target_sources(TheMindRobot PRIVATE
        TheMindRobot/src/GameState.h
        TheMindRobot/src/parser.h
        src/queue.h
)

set_target_properties(TheMindServeur PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/server)
set_target_properties(TheMindClient PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/client)
set_target_properties(TheMindRobot PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/robot)

set(SCRIPTS_DIR ${CMAKE_SOURCE_DIR}/scripts)
add_custom_command(TARGET TheMindServeur POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${SCRIPTS_DIR} ${CMAKE_BINARY_DIR}/bin/server/scripts
)
set(RESSOURCES_DIR ${CMAKE_SOURCE_DIR}/ressources)
add_custom_command(TARGET TheMindServeur POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESSOURCES_DIR} ${CMAKE_BINARY_DIR}/bin/server/ressources
)
set(CLIENT_RESSOURCES_DIR ${CMAKE_SOURCE_DIR}/TheMindClient/ressources)
add_custom_command(TARGET TheMindClient POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CLIENT_RESSOURCES_DIR} ${CMAKE_BINARY_DIR}/bin/client/ressources
)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/server/pdf)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/server/datas)
file(WRITE ${CMAKE_BINARY_DIR}/bin/server/datas/rank.dat "")