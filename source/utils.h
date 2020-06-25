/*
 * utils.h
 *
 * Copyright (c) 2020, DarkMatterCore <pabloacurielz@gmail.com>.
 *
 * This file is part of ww-43db-patcher (https://github.com/DarkMatterCore/ww-43db-patcher).
 *
 * ww-43db-patcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.0.
 *
 * ww-43db-patcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <dirent.h>
#include <gccore.h>
#include <ogc/machine/processor.h>
#include <wiiuse/wpad.h>

#define APP_NAME                        "ww-43db-patcher"
#define VERSION                         "0.1"

/* These control the behaviour of aspect ratio database patching. */
#define BACKUP_U8_ARCHIVE
//#define DISPLAY_ARDB_ENTRIES

#define ERROR_MSG(...)                  utilsPrintErrorMessage(__func__, ##__VA_ARGS__)

#define MEMBER_SIZE(type, member)       sizeof(((type*)NULL)->member)

#define MAX_ELEMENTS(x)                 ((sizeof((x))) / (sizeof((x)[0])))

#define ALIGN_DOWN(x, y)                ((x) & ~((y) - 1))
#define ALIGN_UP(x, y)                  ((((y) - 1) + (x)) & ~((y) - 1))
#define IS_ALIGNED(x, y)                (((x) & ((y) - 1)) == 0)

#define TITLE_UPPER(x)                  ((u32)((x) >> 32))
#define TITLE_LOWER(x)                  ((u32)(x))
#define TITLE_ID(x, y)                  (((u64)(x) << 32) | (y))

#define SYSTEM_MENU_TID                 TITLE_ID(1, 2)

/// Flags a function as (always) inline.
#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE                   __attribute__((always_inline)) static inline
#endif

typedef enum {
    UtilsInputType_Down = 0,
    UtilsInputType_Held = 1
} UtilsInputType;

void *utilsAllocateMemory(size_t size);

void utilsPrintErrorMessage(const char *func_name, const char *fmt, ...);

bool utilsIsWiiU(void);

void utilsReboot(void);

void utilsInitPads(void);
u32 utilsGetInput(int type);

ALWAYS_INLINE void utilsWaitForButtonPress(void)
{
    while(true)
    {
        if (utilsGetInput(UtilsInputType_Down) != 0) break;
    }
}

ALWAYS_INLINE void utilsClearScreen(void)
{
    printf("\x1b[2J");
}

void utilsInitConsole(bool vwii);
void utilsPrintHeadline(void);

signed_blob *utilsGetSignedTMDFromTitle(u64 title_id, u32 *out_size);

ALWAYS_INLINE tmd *utilsGetTMDFromSignedBlob(signed_blob *stmd)
{
    if (!stmd || !IS_VALID_SIGNATURE(stmd)) return NULL;
    return (tmd*)((u8*)stmd + SIGNATURE_SIZE(stmd));
}

void *utilsReadFileFromFlashFileSystem(const char *path, u32 *out_size);
bool utilsWriteDataToFlashFileSystemFile(const char *path, void *buf, u32 size);

#ifdef BACKUP_U8_ARCHIVE
bool utilsMountSdCard(void);
void utilsUnmountSdCard(void);
#endif

#endif /* __UTILS_H__ */
