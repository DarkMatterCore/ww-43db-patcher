/*
 * u8.c
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

#include "utils.h"
#include "u8.h"

#define U8_FILE_ALIGNMENT   0x20

static U8Node *u8GetChildNodeByName(U8Context *ctx, U8Node *dir_node, u32 *node_idx, const char *name, u8 type);

bool u8ContextInit(void *buf, U8Context *ctx)
{
    if (!buf || !ctx)
    {
        ERROR_MSG("Invalid parameters!");
        return false;
    }
    
    u8 *u8_buf = (u8*)buf;
    U8Header u8_header = {0};
    U8Node root_node = {0}, *nodes = NULL;
    u32 node_count = 0, node_section_size = 0, str_table_size = 0;
    char *str_table = NULL;
    bool success = false;
    
    /* Read U8 header. */
    memcpy(&u8_header, u8_buf, sizeof(U8Header));
    
    /* Check header fields. */
    if (u8_header.magic != U8_MAGIC || u8_header.root_node_offset <= (u32)sizeof(U8Header) || u8_header.node_info_block_size <= (u32)sizeof(U8Node) || \
        u8_header.data_offset != ALIGN_UP(u8_header.root_node_offset + u8_header.node_info_block_size, 0x40))
    {
        ERROR_MSG("Invalid U8 header!");
        return false;
    }
    
    /* Read root U8 node. */
    memcpy(&root_node, u8_buf + u8_header.root_node_offset, sizeof(U8Node));
    
    /* Check root U8 node. */
    if (root_node.properties.type != U8NodeType_Directory || root_node.properties.name_offset != 0 || root_node.data_offset != 0 || root_node.size <= 1)
    {
        ERROR_MSG("Invalid root U8 node!");
        return false;
    }
    
    /* Calculate node section size. */
    node_count = root_node.size;
    node_section_size = (u32)(sizeof(U8Node) * node_count);
    if (node_section_size >= u8_header.node_info_block_size)
    {
        ERROR_MSG("Node section size exceeds node info block size in U8 header!");
        return false;
    }
    
    /* Calculate U8 string table size. */
    str_table_size = (u8_header.node_info_block_size - node_section_size);
    if ((node_section_size + str_table_size) != u8_header.node_info_block_size)
    {
        ERROR_MSG("Node info block size in U8 header doesn't match calculated node section and string table sizes!");
        return false;
    }
    
    /* Allocate memory for the U8 nodes. */
    nodes = (U8Node*)utilsAllocateMemory(node_count * sizeof(U8Node));
    if (!nodes)
    {
        ERROR_MSG("Error allocating memory for U8 nodes!");
        return false;
    }
    
    /* Read all U8 nodes. */
    memcpy(nodes, u8_buf + u8_header.root_node_offset, node_count * sizeof(U8Node));
    
    /* Allocate memory for the U8 string table. */
    str_table = (char*)utilsAllocateMemory(str_table_size * sizeof(char));
    if (!str_table)
    {
        ERROR_MSG("Error allocating memory for U8 string table!");
        goto out;
    }
    
    /* Read U8 string table. */
    memcpy(str_table, u8_buf + u8_header.root_node_offset + (node_count * sizeof(U8Node)), str_table_size);
    
    /* Check all U8 nodes. */
    for(u32 i = 1; i < node_count; i++)
    {
        u32 node_number = (i + 1);
        
        /* Check node type. */
        if (nodes[i].properties.type != U8NodeType_File && nodes[i].properties.type != U8NodeType_Directory)
        {
            ERROR_MSG("Invalid entry type for U8 node #%u! (0x%x).", node_number, nodes[i].properties.type);
            goto out;
        }
        
        /* Check name offset. */
        if (nodes[i].properties.name_offset >= str_table_size)
        {
            ERROR_MSG("Name offset for U8 node #%u exceeds string table size!", node_number);
            goto out;
        }
        
        /* Check name. */
        if (!strlen(str_table + nodes[i].properties.name_offset))
        {
            ERROR_MSG("Empty name for U8 node #%u!", node_number);
            goto out;
        }
        
        /* Check data offset. */
        /* Files: check if the data offset matches the current value for the calculated file offset. */
        /* Directories: check if the data offset is equal to or greater than the node count, or if the node it points to isn't a directory node. */
        if ((nodes[i].properties.type == U8NodeType_File && nodes[i].data_offset < u8_header.data_offset) || \
            (nodes[i].properties.type == U8NodeType_Directory && (nodes[i].data_offset >= node_count || nodes[nodes[i].data_offset].properties.type != U8NodeType_Directory)))
        {
            ERROR_MSG("Invalid data offset for U8 node #%u! (0x%x).", node_number, nodes[i].data_offset);
            goto out;
        }
        
        if (nodes[i].properties.type == U8NodeType_Directory)
        {
            /* Check if the size value points to a node number *lower* than this directory's node number, or if it exceeds the total node count. */
            /* We could be dealing with an empty directory, so don't check if the size value is equal to this directory's node number. */
            if (nodes[i].size < node_number || nodes[i].size > node_count)
            {
                ERROR_MSG("Invalid end node number value for U8 node #%u! (0x%x).", node_number, nodes[i].size);
                goto out;
            }
        }
    }
    
    /* Update output context. */
    ctx->u8_buf = u8_buf;
    memcpy(&(ctx->u8_header), &u8_header, sizeof(U8Header));
    ctx->node_count = node_count;
    ctx->nodes = nodes;
    ctx->str_table = str_table;
    
    success = true;
    
out:
    if (!success)
    {
        if (str_table) free(str_table);
        if (nodes) free(nodes);
    }
    
    return success;
}

void u8ContextFree(U8Context *ctx)
{
    if (!ctx) return;
    if (ctx->nodes) free(ctx->nodes);
    if (ctx->str_table) free(ctx->str_table);
    memset(ctx, 0, sizeof(U8Context));
}

U8Node *u8GetDirectoryNodeByPath(U8Context *ctx, const char *path, u32 *out_node_idx)
{
    u32 path_len = 0;
    char *path_dup = NULL, *pch = NULL;
    U8Node *dir_node = NULL;
    u32 node_idx = 0;
    
    if (!ctx || !ctx->str_table || !path || *path != '/' || !(path_len = strlen(path)) || !(dir_node = u8GetNodeByOffset(ctx, 0)))
    {
        ERROR_MSG("Invalid parameters!");
        return NULL;
    }
    
    /* Check if the root directory was requested. */
    if (path_len == 1)
    {
        if (out_node_idx) *out_node_idx = 0;
        return dir_node;
    }
    
    /* Duplicate path to avoid problems with strtok(). */
    if (!(path_dup = strdup(path)))
    {
        ERROR_MSG("Unable to duplicate input path!");
        return NULL;
    }
    
    pch = strtok(path_dup, "/");
    if (!pch)
    {
        ERROR_MSG("Failed to tokenize input path!");
        dir_node = NULL;
        goto out;
    }
    
    while(pch)
    {
        if (!(dir_node = u8GetChildNodeByName(ctx, dir_node, &node_idx, pch, U8NodeType_Directory)))
        {
            ERROR_MSG("Failed to retrieve directory node by name!");
            goto out;
        }
        
        pch = strtok(NULL, "/");
    }
    
    if (out_node_idx) *out_node_idx = node_idx;
    
out:
    if (path_dup) free(path_dup);
    
    return dir_node;
}

U8Node *u8GetFileNodeByPath(U8Context *ctx, const char *path, u32 *out_node_idx)
{
    u32 path_len = 0;
    char *path_dup = NULL, *filename = NULL;
    U8Node *dir_node = NULL, *file_node = NULL;
    u32 node_idx = 0;
    
    if (!ctx || !ctx->str_table || !path || *path != '/' || (path_len = strlen(path)) <= 1)
    {
        ERROR_MSG("Invalid parameters!");
        return NULL;
    }
    
    /* Duplicate path. */
    if (!(path_dup = strdup(path)))
    {
        ERROR_MSG("Unable to duplicate input path!");
        return NULL;
    }
    
    /* Remove any trailing slashes. */
    while(path_dup[path_len - 1] == '/')
    {
        path_dup[path_len - 1] = '\0';
        path_len--;
    }
    
    /* Safety check. */
    if (!path_len || !(filename = strrchr(path_dup, '/')))
    {
        ERROR_MSG("Invalid input path!");
        goto out;
    }
    
    /* Remove leading slash and adjust filename string pointer. */
    *filename++ = '\0';
    
    /* Retrieve directory node. */
    /* If the first character is NULL, then just retrieve the root directory node. */
    if (!(dir_node = (*path_dup ? u8GetDirectoryNodeByPath(ctx, path_dup, &node_idx) : u8GetNodeByOffset(ctx, 0))))
    {
        ERROR_MSG("Failed to retrieve directory node!");
        goto out;
    }
    
    /* Retrieve file node. */
    if (!(file_node = u8GetChildNodeByName(ctx, dir_node, &node_idx, filename, U8NodeType_File)))
    {
        ERROR_MSG("Failed to retrieve file node by name!");
        goto out;
    }
    
    if (out_node_idx) *out_node_idx = node_idx;
    
out:
    if (path_dup) free(path_dup);
    
    return file_node;
}

u8 *u8LoadFileData(U8Context *ctx, U8Node *file_node, u32 *out_size)
{
    if (!ctx || !ctx->u8_buf || !ctx->u8_header.data_offset || !file_node || file_node->properties.type != U8NodeType_File || file_node->data_offset < ctx->u8_header.data_offset || \
        !file_node->size || !out_size)
    {
        ERROR_MSG("Invalid parameters!");
        return NULL;
    }
    
    /* Allocate memory for the file buffer. */
    u8 *buf = (u8*)utilsAllocateMemory(file_node->size);
    if (!buf)
    {
        ERROR_MSG("Error allocating memory for file buffer!");
        return NULL;
    }
    
    /* Read file data. */
    memcpy(buf, ctx->u8_buf + file_node->data_offset, file_node->size);
    *out_size = file_node->size;
    
    return buf;
}

bool u8SaveFileData(U8Context *ctx, U8Node *file_node, void *buf, u32 size)
{
    u8 *buf_u8 = NULL;
    
    if (!ctx || !ctx->u8_buf || !ctx->u8_header.data_offset || !file_node || file_node->properties.type != U8NodeType_File || file_node->data_offset < ctx->u8_header.data_offset || \
        !file_node->size || !(buf_u8 = (u8*)buf) || !size || size != file_node->size)
    {
        ERROR_MSG("Invalid parameters!");
        return false;
    }
    
    /* Save file data. */
    memcpy(ctx->u8_buf + file_node->data_offset, buf_u8, file_node->size);
    
    return true;
}

static U8Node *u8GetChildNodeByName(U8Context *ctx, U8Node *dir_node, u32 *node_idx, const char *name, u8 type)
{
    u32 name_len = 0;
    
    if (!ctx || !ctx->nodes || !ctx->str_table || !dir_node || dir_node->properties.type != U8NodeType_Directory || !node_idx || *node_idx >= ctx->node_count || (*node_idx + 1) >= dir_node->size || \
        !name || !(name_len = strlen(name)) || (type != U8NodeType_File && type != U8NodeType_Directory)) return NULL;
    
    for(u32 i = (*node_idx + 1); i < dir_node->size; i++)
    {
        char *node_name = (ctx->str_table + ctx->nodes[i].properties.name_offset);
        
        if (ctx->nodes[i].properties.type != type || strlen(node_name) != name_len) continue;
        
        if (!strcmp(node_name, name))
        {
            *node_idx = i;
            return &(ctx->nodes[i]);
        }
    }
    
    return NULL;
}
