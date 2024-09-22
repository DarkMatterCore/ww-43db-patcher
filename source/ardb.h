/*
 * ardb.h
 *
 * Copyright (c) 2020-2024, DarkMatterCore <pabloacurielz@gmail.com>.
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

#ifndef __ARDB_H__
#define __ARDB_H__

#define ARDB_MAGIC              (u32)0x34334442 /* "43DB". */

#define ARDB_WC24_EVC_ENTRY     0x48414A        /* "HAJ" - Everybody Votes Channel. */
#define ARDB_WC24_CMOC_ENTRY    0x484150        /* "HAP" - Check Mii Out Channel. */

typedef struct {
    u32 magic;          ///< ARDB_MAGIC.
    u32 version;        ///< Database version number.
    u32 entry_count;    ///< Entry count.
    u32 reserved;       ///< Reserved.
    u32 entries[];      ///< C99 flexible array with entries.
} AspectRatioDatabase;

SIZE_ASSERT(AspectRatioDatabase, 0x10);

typedef enum {
    AspectRatioDatabaseType_Disc            = 0,
    AspectRatioDatabaseType_VirtualConsole  = 1,
    AspectRatioDatabaseType_WiiWare         = 2,
    AspectRatioDatabaseType_Count           = 3
} AspectRatioDatabaseType;

/// Patches an aspect ratio database stored inside the System Menu's U8 archive by removing the desired title IDs from its records.
/// The entries from the provided u32 array must use a 3-byte representation of the desired title IDs, ignoring the last byte value.
bool ardbPatchDatabaseFromSystemMenuArchive(u8 type, const u32 *entries, const u32 entry_count);

#ifdef BACKUP_U8_ARCHIVE
/// Restores a previously created System Menu U8 archive from the inserted SD card.
bool ardbRestoreSystemMenuArchive(void);
#endif  /* BACKUP_U8_ARCHIVE */

#endif /* __ARDB_H__ */
