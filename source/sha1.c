/*
 * sha1.c
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

#include "utils.h"
#include "sha1.h"

static bool g_sha1EngineInitialized = false;

static bool sha1EngineInitialize(void);
static void sha1EngineClose(void);

#define SHA_ENGINE_WRAPPER(func, ...) \
    bool ret = false; \
    if (sha1EngineInitialize()) ret = (func(__VA_ARGS__) >= 0); \
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

    sha_context ctx ATTRIBUTE_ALIGN(32) = {0};
    sha1 hash ATTRIBUTE_ALIGN(32) = {0};

    u8 *src_u8 = NULL;
    bool src_aligned = IS_ALIGNED((u32)src, 64);

    /* Handle data alignment (if needed). */
    if (!src_aligned)
    {
        u8 *tmp = utilsAllocateMemory(size);
        if (!tmp) goto end;

        memcpy(tmp, src, size);

        src_u8 = tmp;
    } else {
        src_u8 = (u8*)src;
    }

    /* Initialize SHA engine and context. */
    if (!sha1EngineInitialize() || SHA_InitializeContext(&ctx) < 0) goto end;

    /* Calculate SHA checksum. */
    ret = (SHA_Calculate(&ctx, src_u8, size, hash) >= 0);
    if (!ret) goto end;

    /* Copy checksum to destination pointer. */
    memcpy(dst, hash, sizeof(hash));

end:
    sha1EngineClose();

    if (!src_aligned && src_u8) free(src_u8);

    return ret;
}

static bool sha1EngineInitialize(void)
{
    if (g_sha1EngineInitialized) return true;
    g_sha1EngineInitialized = (SHA_Init() >= 0);
    return g_sha1EngineInitialized;
}

static void sha1EngineClose(void)
{
    if (!g_sha1EngineInitialized) return;
    SHA_Close();
    g_sha1EngineInitialized = false;
}
