/*
 * ardb.h
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

#ifndef __ARDB_H__
#define __ARDB_H__

#define ARDB_MAGIC    (u32)0x34334442 /* "43DB". */

typedef struct {
    u32 magic;          ///< ARDB_MAGIC.
    u32 version;        ///< Database version number.
    u32 entry_count;    ///< Entry count.
    u32 reserved;       ///< Reserved.
    u32 entries[];      ///< C99 flexible array with entries.
} AspectRatioDatabase;

typedef enum {
    AspectRatioDatabaseType_Disc           = 0,
    AspectRatioDatabaseType_VirtualConsole = 1,
    AspectRatioDatabaseType_WiiWare        = 2
} AspectRatioDatabaseType;

/// Patches an aspect ratio database stored inside the System Menu's U8 archive.
bool ardbPatchDatabaseFromSystemMenuArchive(u8 type);

#ifdef BACKUP_U8_ARCHIVE
/// Restores a previously created System Menu U8 archive from the inserted SD card.
bool ardbRestoreSystemMenuArchive(void);
#endif  /* BACKUP_U8_ARCHIVE */

#endif /* __ARDB_H__ */
