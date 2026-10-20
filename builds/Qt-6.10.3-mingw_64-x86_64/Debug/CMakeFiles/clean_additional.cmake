# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\final_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\final_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\game_core_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\game_core_autogen.dir\\ParseCache.txt"
  "final_autogen"
  "game_core_autogen"
  )
endif()
