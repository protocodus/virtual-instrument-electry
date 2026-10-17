# Render the README screenshot through the processor suite, then prove a whole
# PNG of the editor's size landed. Removing the old file first is what makes
# the check meaningful: it can only exist afterwards because this run wrote it.
#
# -DELECTRY_PROCESSOR_TESTS=<ElectryPluginProcessorTests> -DELECTRY_SCREENSHOT=<png>

foreach(required ELECTRY_PROCESSOR_TESTS ELECTRY_SCREENSHOT)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

file(REMOVE "${ELECTRY_SCREENSHOT}")
set(ENV{ELECTRY_EDITOR_SNAPSHOT} "${ELECTRY_SCREENSHOT}")
execute_process(COMMAND "${ELECTRY_PROCESSOR_TESTS}" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "the processor suite failed (${result}); its screenshot is not published")
endif()
if(NOT EXISTS "${ELECTRY_SCREENSHOT}")
    message(FATAL_ERROR "the processor suite did not write ${ELECTRY_SCREENSHOT}")
endif()

file(SIZE "${ELECTRY_SCREENSHOT}" size)
file(READ "${ELECTRY_SCREENSHOT}" header LIMIT 24 HEX)
math(EXPR trailer_offset "${size} - 8")
file(READ "${ELECTRY_SCREENSHOT}" trailer OFFSET ${trailer_offset} HEX)
if(NOT header MATCHES "^89504e470d0a1a0a0000000d49484452")
    message(FATAL_ERROR "${ELECTRY_SCREENSHOT} is not a PNG")
endif()
# Every PNG ends with an empty IEND chunk and its constant CRC; without it the
# encoder did not finish.
if(NOT trailer STREQUAL "49454e44ae426082")
    message(FATAL_ERROR "${ELECTRY_SCREENSHOT} has no IEND trailer")
endif()

# The editor is a fixed 1180 x 910 component (PluginEditor.cpp).
string(SUBSTRING "${header}" 32 8 width_hex)
string(SUBSTRING "${header}" 40 8 height_hex)
math(EXPR width "0x${width_hex}")
math(EXPR height "0x${height_hex}")
if(NOT width EQUAL 1180 OR NOT height EQUAL 910)
    message(FATAL_ERROR "${ELECTRY_SCREENSHOT} is ${width} x ${height}, expected 1180 x 910")
endif()
message(STATUS "Rendered ${ELECTRY_SCREENSHOT} (${width} x ${height}, ${size} bytes)")
