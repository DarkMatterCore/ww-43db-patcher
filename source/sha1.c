/*
 * sha1.c
 *
 * Copyright (c) 2023-2024, DarkMatterCore <pabloacurielz@gmail.com>.
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
#include "sha1.h"

static bool g_sha1EngineInitialized = false;

static bool sha1EngineInitialize(void);
static void sha1EngineClose(void);

#define SHA_ENGINE_WRAPPER(func, ...) \
    bool ret = false; \
    if (sha1EngineInitialize()) { \
        s32 rc = func(__VA_ARGS__); \
        ret = (rc >= 0); \
        if (!ret) ERROR_MSG(#func "() failed! (%d).", rc); \
    } \
    sha1EngineClose(); \
    return ret;

bool sha1ContextCreate(sha_context *ctx)
{
    SHA_ENGINE_WRAPPER(SHA_InitializeContext, ctx);
}

bool sha1ContextUpdate(sha_context *ctx, const void *src, const u32 size)
{
    SHA_ENGINE_WRAPPER(SHA_Input, ctx, src, size);
}

bool sha1ContextGetHash(sha_context *ctx, const void *src, const u32 size, void *dst)
{
    SHA_ENGINE_WRAPPER(SHA_Calculate, ctx, src, size, dst);
}

#undef SHA_ENGINE_WRAPPER

bool sha1CalculateHash(const void *src, const u32 size, void *dst)
{
    if (!src || !size || !dst) return false;

    bool ret = false;

    s32 rc = 0;

    u8 *src_u8 = NULL;
    bool src_aligned = IS_ALIGNED((u32)src, 64);

    sha_context ctx ATTRIBUTE_ALIGN(32) = {0};
    u8 hash[SHA1_HASH_SIZE] ATTRIBUTE_ALIGN(32) = {0};

    /* Handle data alignment (if needed). */
    if (!src_aligned)
    {
        u8 *tmp = utilsAllocateMemory(size);
        if (!tmp)
        {
            ERROR_MSG("Failed to allocate memory for aligned 0x%X-byte long buffer!", size);
            goto end;
        }

        memcpy(tmp, src, size);

        src_u8 = tmp;
    } else {
        src_u8 = (u8*)src;
    }

    /* Initialize SHA engine. */
    if (!sha1EngineInitialize()) goto end;

    /* Initialize SHA context. */
    rc = SHA_InitializeContext(&ctx);
    if (rc < 0)
    {
        ERROR_MSG("SHA_InitializeContext() failed! (%d).", rc);
        goto end;
    }

    /* Calculate SHA checksum. */
    rc = SHA_Calculate(&ctx, src_u8, size, hash);
    if (rc < 0)
    {
        ERROR_MSG("SHA_InitializeContext() failed! (%d).", rc);
        goto end;
    }

    /* Copy checksum to destination pointer. */
    memcpy(dst, hash, sizeof(hash));

    /* Update return value. */
    ret = true;

end:
    /* Close SHA engine. */
    sha1EngineClose();

    /* Free allocated buffer, if needed. */
    if (!src_aligned && src_u8) free(src_u8);

    return ret;
}

static bool sha1EngineInitialize(void)
{
    if (g_sha1EngineInitialized) return true;

    s32 rc = SHA_Init();
    g_sha1EngineInitialized = (rc >= 0);
    if (!g_sha1EngineInitialized) ERROR_MSG("SHA_Init() failed! (%d).", rc);

    return g_sha1EngineInitialized;
}

static void sha1EngineClose(void)
{
    if (!g_sha1EngineInitialized) return;

    SHA_Close();

    g_sha1EngineInitialized = false;
}
