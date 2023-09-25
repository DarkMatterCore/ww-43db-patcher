/*
 * sha1.h
 *
 * Copyright (c) 2023, DarkMatterCore <pabloacurielz@gmail.com>.
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

#ifndef __SHA1_H__
#define __SHA1_H__

#include <ogc/sha.h>

#define SHA1_HASH_SIZE 0x20

/// Wrappers for SHA_*() functions within libogc. These make sure the SHA engine is initialized before doing anything.
/// These do not, however, take care of handling I/O alignment.
bool sha1ContextCreate(sha_context *ctx);
bool sha1ContextUpdate(sha_context *ctx, const void *src, const u32 size);
bool sha1ContextGetHash(sha_context *ctx, const void *src, const u32 size, void *dst);

/// Simple all-in-one SHA-1 calculator. Handles I/O alignment if needed.
bool sha1CalculateHash(const void *src, const u32 size, void *dst);

#endif /* __SHA1_H__ */
