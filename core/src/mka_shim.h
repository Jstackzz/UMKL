// Force-included into every file of mka_fbneo.dll: what FBNeo's headless (POSIX-minded) front end expects from its
// platform that MSVC spells differently. Subject to the FB Neo license, like the rest of the DLL.
#pragma once

#ifdef _MSC_VER
#include <string.h>
#include <stdio.h>
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif
#endif

#ifdef __cplusplus
#include <tchar.h>
#ifndef dprintf
#define dprintf(...) ((void)0)
#endif
#endif
