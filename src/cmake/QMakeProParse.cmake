# -------------------
# QMakeProParse.cmake
# -------------------
#
# qmake project parsing utilities
#

# parse_qmake_pro(<PRO_FILE> <PREFIX>)
#
# parse qmake .pro file PRO_FILE and set cmake variables <PREFIX>_<VAR_NAME>
# to content of variable VAR_NAME in the .pro file.
#
# supported qmake syntax:
#  - VARIABLE = values
#  - VARIABLE += values
#
# not (yet) supported:
#  - VARIABLE -= values
#  - scopes
#
function(parse_qmake_pro PRO_FILE PREFIX)
	file(READ ${PRO_FILE} PRO_CONTENT)

	# concatenate lines with backslashes
	string(REGEX REPLACE "\\\\\ *\n" "" PRO_CONTENT "${PRO_CONTENT}")

	# separate lines
	string(REGEX MATCHALL "[^\n]+(\n|$)" PRO_LINES "${PRO_CONTENT}")

	foreach(LINE IN LISTS PRO_LINES)
		string(STRIP "${LINE}" LINE)

		# VARIABLE = some values ...
		string(REGEX MATCH "^([a-zA-Z_]+)\ *=(.*)" VAR_SET "${LINE}")
		if(CMAKE_MATCH_1)
			separate_arguments(VALUES UNIX_COMMAND ${CMAKE_MATCH_2})
			set(${PREFIX}_${CMAKE_MATCH_1} "${VALUES}" PARENT_SCOPE)
		endif()

		# VARIABLE += some values ...
		string(REGEX MATCH "^([a-zA-Z_]+)\ *\\+=(.*)" VAR_SET "${LINE}")
		if(CMAKE_MATCH_1)
			separate_arguments(VALUES UNIX_COMMAND ${CMAKE_MATCH_2})
			set(VAR_NAME ${PREFIX}_${CMAKE_MATCH_1})
			set(${VAR_NAME} "${${VAR_NAME}}" "${VALUES}" PARENT_SCOPE)
		endif()

	endforeach()
endfunction()