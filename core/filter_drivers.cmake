# Removes drivers from gamelist.pl's driverlist.h: their declaration, their entry in the driver table and their
# name-to-file line. The driver code still compiles (it shares the board code); it just never appears in the core.
#   cmake -DIN=all.h -DOUT=driverlist.h "-DEXCLUDE=umk3w;umk3wp" -P filter_drivers.cmake
file(READ "${IN}" text)
foreach(name IN LISTS EXCLUDE)
    string(SUBSTRING "${name}" 0 1 first)
    string(TOUPPER "${first}" first)
    string(SUBSTRING "${name}" 1 -1 rest)
    set(sym "BurnDrv${first}${rest}")
    string(REGEX REPLACE "[^\n]*${sym}[,;][^\n]*\n" "" text "${text}")
    string(REGEX REPLACE "[^\n]*\\{ \"${name}\",[^\n]*\n" "" text "${text}")
endforeach()
file(WRITE "${OUT}.tmp" "${text}")
file(COPY_FILE "${OUT}.tmp" "${OUT}" ONLY_IF_DIFFERENT)
file(REMOVE "${OUT}.tmp")
