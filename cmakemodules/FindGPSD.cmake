set(GPSD_FOUND "NO")

set(CHUNKY_INSTALLED_GPSD_ON_HIS_MAC_IN /Users/chunky/gpsd/)

find_path(GPSD_INCLUDE_DIR gps.h
	/usr/include/
	/usr/local/include/
	${CHUNKY_INSTALLED_GPSD_ON_HIS_MAC_IN}/include/
)

find_library(GPSD_LIBRARY NAMES gps PATHS
	/usr/lib
	/usr/local/lib
	${CHUNKY_INSTALLED_GPSD_ON_HIS_MAC_IN}/lib/
)


if(GPSD_LIBRARY AND GPSD_INCLUDE_DIR)
  set(GPSD_FOUND "YES")
endif(GPSD_LIBRARY AND GPSD_INCLUDE_DIR)

if(GPSD_FOUND)
  if(NOT GPSD_FIND_QUIETLY)
    message(STATUS "Found GPSD: ${GPSD_LIBRARY}")
  endif(NOT GPSD_FIND_QUIETLY)
else(GPSD_FOUND)
  if(GPSD_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find GPSD Library")
  endif(GPSD_FIND_REQUIRED)
endif(GPSD_FOUND)

