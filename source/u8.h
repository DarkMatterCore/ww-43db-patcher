/*
 * u8.h
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

#ifndef __U8_H__
#define __U8_H__

#define U8_MAGIC    (u32)0x55AA382D /* "U.8-". */

typedef struct {
    u32 magic;                  ///< U8_MAGIC.
    u32 root_node_offset;       ///< Root node offset, relative to the start of this header.
    u32 node_info_block_size;   ///< Node table size + string table size, starting from the root node offset.
    u32 data_offset;            ///< Root node offset + node info block size, aligned to 0x40. Relative to the start of this header.
} U8Header;

typedef enum {
    U8NodeType_File      = 0,
    U8NodeType_Directory = 1
} U8NodeType;

typedef struct {
    u32 type        : 8;    ///< U8NodeType.
    u32 name_offset : 24;   ///< Offset to node name. Relative to the start of the string table.
} U8NodeProperties;

typedef struct {
    U8NodeProperties properties;    ///< Using a bitfield because of the odd name_offset field size.
    u32 data_offset;                ///< Files: offset to file data (relative to the start of the U8 header). Directories: parent dir node index (0-based).
    u32 size;                       ///< Files: data size. Directories: node number from the last file inside this directory (root node is number 1).
} U8Node;

typedef struct {
    u8 *u8_buf;
    U8Header u8_header;
    u32 node_count;
    U8Node *nodes;
    char *str_table;
} U8Context;

/// Initializes an U8 context.
bool u8ContextInit(void *buf, U8Context *ctx);

/// Frees an U8 context.
void u8ContextFree(U8Context *ctx);

/// Retrieves an U8 directory node by its path.
/// Its index is saved to the out_node_idx pointer (if provided).
U8Node *u8GetDirectoryNodeByPath(U8Context *ctx, const char *path, u32 *out_node_idx);

/// Retrieves an U8 file node by its path.
/// Its index is saved to the out_node_idx pointer (if provided).
U8Node *u8GetFileNodeByPath(U8Context *ctx, const char *path, u32 *out_node_idx);

/// Loads file data from an U8 file node into memory.
/// The returned pointer must be freed by the user.
u8 *u8LoadFileData(U8Context *ctx, U8Node *file_node, u32 *out_size);

/// Saves file data into an U8 archive loaded into memory.
bool u8SaveFileData(U8Context *ctx, U8Node *file_node, void *buf, u32 size);

/// Retrieves an U8 node by its offset.
ALWAYS_INLINE U8Node *u8GetNodeByOffset(U8Context *ctx, u32 offset)
{
    u32 node_idx = 0;
    if (!ctx || !ctx->nodes || !IS_ALIGNED(offset, sizeof(U8Node)) || (node_idx = (u32)(offset / sizeof(U8Node))) >= ctx->node_count) return NULL;
    return &(ctx->nodes[node_idx]);
}

#endif /* __U8_H__ */
