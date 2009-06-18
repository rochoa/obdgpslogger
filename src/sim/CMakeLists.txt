INCLUDE_DIRECTORIES(
	.
	../obdinfo/
)

# Build the generators
# To add a generator:
#  1) Write generators/gen_{name}.c
#  2) Add {name} to OBDSIM_GENERATORS below
#  3) Add the libraries to the IF/ELSEIF below
SET(OBDSIM_GENERATORS
	logger
	random
)

FOREACH(DATAGEN ${OBDSIM_GENERATORS})
	ADD_LIBRARY(ckobdsim_${DATAGEN} STATIC
			./generators/gen_${DATAGEN}.c)
ENDFOREACH(DATAGEN ${OBDSIM_GENERATORS})

SET(OBD_SIM_GENERATOR "logger" CACHE STRING
		"Data Generator for obdsim. Choices: ${OBDSIM_GENERATORS}"
)

IF(OBD_SIM_GENERATOR MATCHES "logger")
	SET(GENERATOR_LIBS ckobdsim_logger cksqlite)
ELSEIF(OBD_SIM_GENERATOR MATCHES "random")
	SET(GENERATOR_LIBS ckobdsim_random)
ENDIF(OBD_SIM_GENERATOR MATCHES "logger")
# Generators are setup now


INCLUDE(CheckSymbolExists)
INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(ptsname_r HAVE_PTSNAME_R)
IF(HAVE_PTSNAME_R)
	ADD_DEFINITIONS(-DHAVE_PTSNAME_R)
ENDIF(HAVE_PTSNAME_R)

FILE(GLOB OBDSIM_SRCS
	*.c
)

SET(OBDSIM_LIBS
	ckobdinfo ckobdconfigfile
	${GENERATOR_LIBS}
)

IF("${CMAKE_SYSTEM}" MATCHES "Linux")
	SET(OBDSIM_LIBS ${OBDSIM_LIBS} pthread dl)
ENDIF("${CMAKE_SYSTEM}" MATCHES "Linux")

ADD_EXECUTABLE(obdsim ${OBDSIM_SRCS})

TARGET_LINK_LIBRARIES(obdsim ${OBDSIM_LIBS})

INSTALL(TARGETS obdsim
	RUNTIME DESTINATION bin)

