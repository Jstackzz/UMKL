# mka_fbneo.dll: FBNeo's emulation core with only the Midway TMS34010 drivers (Y-Unit, T-Unit, Wolf Unit), its
# headless front end, and core/src/mka_core.cpp (in-memory machine states for rollback). Everything in this DLL is
# subject to the FB Neo license (external/FBNeo/src/license.txt): non-commercial, sources of changes public.
# The host (MKArcade.exe) only loads it, so the host's own code stays MIT.
cmake_minimum_required(VERSION 3.24)
project(mka_core C CXX)

set(FB "${CMAKE_CURRENT_SOURCE_DIR}/../external/FBNeo/src")
if(NOT EXISTS "${FB}/burn/burn.cpp")
    message(FATAL_ERROR "FBNeo source not found at ${FB} (clone it, docs/SETUP.md)")
endif()
find_program(PERL perl HINTS "C:/Program Files/Git/usr/bin" REQUIRED)

set(GEN "${CMAKE_CURRENT_BINARY_DIR}/generated")
# FBNeo's sdl and headless burner folders carry a POSIX tchar.h. MSVC also searches the folders of the files that
# include a header, so those sources are compiled from copies here, where only the Windows SDK's tchar.h is found.
set(SHADOW "${CMAKE_CURRENT_BINARY_DIR}/shadow")
file(MAKE_DIRECTORY "${GEN}" "${SHADOW}")

# --- the drivers: MK1 (Y-Unit and T-Unit), MK2 (T-Unit), MK3 and UMK3 (Wolf Unit), with their clones
set(DRIVER_FILES
    "${FB}/burn/drv/d_parent.cpp"
    "${FB}/burn/drv/midway/d_yunit.cpp"
    "${FB}/burn/drv/midway/d_tunit.cpp"
    "${FB}/burn/drv/midway/d_wunit.cpp")
string(REPLACE ";" "\n" _manifest "${DRIVER_FILES}")
file(WRITE "${GEN}/driverlist_manifest.txt" "${_manifest}\n")
# sets left out of the core: UMK3's WaveNet Edition (Midway's 1997 network version, with its own modem board)
set(EXCLUDED_DRIVERS umk3w umk3wp)
add_custom_command(
    OUTPUT "${GEN}/driverlist.h"
    COMMAND "${PERL}" "${FB}/dep/scripts/gamelist.pl" -o "${GEN}/driverlist_all.h" -l "${GEN}/gamelist.txt"
            -f "${GEN}/driverlist_manifest.txt"
    COMMAND "${CMAKE_COMMAND}" "-DIN=${GEN}/driverlist_all.h" "-DOUT=${GEN}/driverlist.h"
            "-DEXCLUDE=${EXCLUDED_DRIVERS}" -P "${CMAKE_CURRENT_SOURCE_DIR}/filter_drivers.cmake"
    DEPENDS ${DRIVER_FILES} "${FB}/dep/scripts/gamelist.pl" "${CMAKE_CURRENT_SOURCE_DIR}/filter_drivers.cmake"
    COMMENT "gamelist.pl: driverlist.h for the Midway drivers")
string(TIMESTAMP _now "%H:%M:%S")
string(TIMESTAMP _today "%b %d %Y")
file(WRITE "${GEN}/build_details.h"
    "#define BUILD_TIME ${_now}\n#define BUILD_DATE ${_today}\n#define BUILD_CHAR ANSI\n"
    "#define BUILD_CPU  X64\n#define BUILD_COMP Visual C++\n")

set(BURN_SRC burn.cpp burn_bitmap.cpp burn_gun.cpp burn_led.cpp burn_memory.cpp burn_pal.cpp burn_sha1.cpp
    burn_shift.cpp burn_sound.cpp burn_sound_c.cpp cheat.cpp debug_track.cpp hiscore.cpp load.cpp
    tilemap_generic.cpp tiles_generic.cpp timer.cpp vector.cpp)
list(TRANSFORM BURN_SRC PREPEND "${FB}/burn/")

set(DEVICE_SRC joyprocess.cpp 6821pia.cpp midcvsd.cpp)
list(TRANSFORM DEVICE_SRC PREPEND "${FB}/burn/devices/")

set(SOUND_SRC dac.cpp hc55516.cpp msm6295.cpp burn_ym2151.cpp ym2151.c burn_ost.cpp samples.cpp)
list(TRANSFORM SOUND_SRC PREPEND "${FB}/burn/snd/")

set(CPU_SRC tms34/tms34010.cpp tms34_intf.cpp adsp2100/adsp2100.cpp adsp2100_intf.cpp m6809/m6809.cpp
    m6809_intf.cpp z80/z80.cpp z80/z80ctc.cpp z80/z80daisy.cpp z80/z80pio.cpp z80_intf.cpp)
list(TRANSFORM CPU_SRC PREPEND "${FB}/cpu/")

# ide.cpp (and burn_hdd.cpp, which needs libchdr) serve only Killer Instinct's hard disk, not built here
set(MIDWAY_SRC dcs2k.cpp midtunit.cpp midwayic.cpp midwunit.cpp narc_sound.cpp williams_adpcm.cpp yawdim_sound.cpp)
list(TRANSFORM MIDWAY_SRC PREPEND "${FB}/burn/drv/midway/")

# shared burner code and the platform-neutral interfaces
set(BURNER_SRC
    burner/conc.cpp burner/cong.cpp burner/dat.cpp burner/gamc.cpp burner/gami.cpp
    burner/image.cpp burner/inputbuf.cpp burner/ioapi.c burner/misc.cpp burner/sshot.cpp burner/state.cpp
    burner/statec.cpp burner/unzip.c burner/zipfn.cpp
    intf/audio/aud_dsp.cpp intf/audio/aud_interface.cpp intf/audio/lowpass2.cpp
    intf/input/inp_interface.cpp intf/interface.cpp
    intf/video/vid_interface.cpp intf/video/vid_support.cpp
    intf/audio/headless/aud_headless.cpp intf/input/headless/inp_headless.cpp intf/video/headless/vid_headless.cpp
    dep/libs/libspng/spng.c dep/libs/zstd/zstddeclib.c)
list(TRANSFORM BURNER_SRC PREPEND "${FB}/")

set(ZLIB_SRC adler32.c compress.c crc32.c deflate.c gzclose.c gzlib.c gzread.c gzwrite.c infback.c inffast.c
    inflate.c inftrees.c trees.c uncompr.c zutil.c)
list(TRANSFORM ZLIB_SRC PREPEND "${FB}/dep/libs/zlib/")

# the headless front end and the burner/sdl files it borrows, compiled from copies (see SHADOW above)
set(SHADOW_SRC)
foreach(f support_paths ips_manager scrn localise_gamelist romdata stringset bzip media inpdipsw replay stated)
    configure_file("${FB}/burner/sdl/${f}.cpp" "${SHADOW}/${f}.cpp" COPYONLY)
    list(APPEND SHADOW_SRC "${SHADOW}/${f}.cpp")
endforeach()
foreach(f headless_main headless_drv headless_misc headless_input headless_memory)
    configure_file("${FB}/burner/headless/${f}.cpp" "${SHADOW}/${f}.cpp" COPYONLY)
    list(APPEND SHADOW_SRC "${SHADOW}/${f}.cpp")
endforeach()
# Copies with a small change, made here so external/FBNeo stays as cloned (docs/SETUP.md lists each one).
function(mka_patched_copy src dst find replace)
    file(READ "${src}" _text)
    string(FIND "${_text}" "${find}" _at)
    if(_at EQUAL -1)
        message(FATAL_ERROR "patch for ${src} no longer applies: '${find}' not found")
    endif()
    string(REPLACE "${find}" "${replace}" _text "${_text}")
    file(WRITE "${dst}.tmp" "${_text}")
    file(COPY_FILE "${dst}.tmp" "${dst}" ONLY_IF_DIFFERENT)
    file(REMOVE "${dst}.tmp")
endfunction()
# cd_interface.cpp: the headless build lists no CD image backend, an empty array MSVC rejects; give it the image one
mka_patched_copy("${FB}/intf/cd/cd_interface.cpp" "${SHADOW}/cd_interface.cpp"
    "#elif defined (_XBOX)" "#elif defined (_XBOX) || defined (BUILD_HEADLESS)")
list(APPEND SHADOW_SRC "${SHADOW}/cd_interface.cpp")

file(MAKE_DIRECTORY "${SHADOW}/inc")
foreach(h burner_headless.h fbheadless.h)
    configure_file("${FB}/burner/headless/${h}" "${SHADOW}/inc/${h}" COPYONLY)
endforeach()

add_library(mka_fbneo SHARED
    ${BURN_SRC} ${DEVICE_SRC} ${SOUND_SRC} ${CPU_SRC} ${MIDWAY_SRC} ${DRIVER_FILES} ${BURNER_SRC} ${ZLIB_SRC}
    ${SHADOW_SRC}
    "${GEN}/driverlist.h"
    src/mka_core.cpp
    src/mka_stubs.cpp
    mka_fbneo.def)

target_include_directories(mka_fbneo PRIVATE
    "${SHADOW}/inc" "${GEN}"
    "${FB}/burn" "${FB}/burn/devices" "${FB}/burn/snd" "${FB}/burn/drv/midway" "${FB}/cpu"
    "${FB}/cpu/tms34" "${FB}/cpu/adsp2100" "${FB}/cpu/m6809" "${FB}/cpu/z80"
    "${FB}/burner" "${FB}/intf" "${FB}/intf/audio" "${FB}/intf/video" "${FB}/intf/input" "${FB}/intf/cd"
    "${FB}/intf/perfcount" "${FB}/dep/libs/zlib" "${FB}/dep/libs/libspng" "${FB}/dep/libs/zstd")

target_compile_definitions(mka_fbneo PRIVATE
    BUILD_HEADLESS SUPPORT_SPLIT_DRIVER BUILD_MIDWAY LSB_FIRST FASTCALL USE_SPEEDHACKS _USE_MATH_DEFINES
    _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS SPNG_STATIC)
target_compile_options(mka_fbneo PRIVATE
    /FI "${CMAKE_CURRENT_SOURCE_DIR}/src/mka_shim.h"
    /W0 /MP /bigobj /utf-8
    $<$<COMPILE_LANGUAGE:CXX>:/Zc:strictStrings->
    $<$<COMPILE_LANGUAGE:CXX>:/permissive>)
set_target_properties(mka_fbneo PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    OUTPUT_NAME mka_fbneo)
target_link_libraries(mka_fbneo PRIVATE winmm)
