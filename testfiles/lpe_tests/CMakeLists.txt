# SPDX-License-Identifier: GPL-2.0-or-later

file(GLOB files "*.svg")
foreach(file ${files})
    set(testname "lpe_${file}")
    add_test(NAME lpe_${file}
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test.sh ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_lpes ${file}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/testfiles/lpe_tests)
endforeach()

