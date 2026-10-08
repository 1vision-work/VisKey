# SPDX-License-Identifier: GPL-3.0-or-later
execute_process(COMMAND ${GEN} ${OUT} RESULT_VARIABLE rc OUTPUT_QUIET)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "snapshot generator failed (${rc})")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${OUT} ${EXPECTED} RESULT_VARIABLE diff)
if(NOT diff EQUAL 0)
  message(FATAL_ERROR "snapshot differs from original engine output: ${OUT} vs ${EXPECTED}")
endif()
