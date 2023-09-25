/*
 * utils.c
 *
 * Copyright (c) 2020-2023, DarkMatterCore <pabloacurielz@gmail.com>.
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

#include <fat.h>
#include <sdcard/wiisd_io.h>

#include "utils.h"

#define BC_NAND_TID TITLE_ID(1, 0x200)

/* Global variables. */

static void *g_xfb = NULL;
static GXRModeObj *g_rmode = NULL;

static u64 g_tmdTitleId ATTRIBUTE_ALIGN(32) = 0;
static u32 g_tmdSize ATTRIBUTE_ALIGN(32) = 0;

static s32 g_isfsFd ATTRIBUTE_ALIGN(32) = 0;
static char g_isfsFilePath[ISFS_MAXPATH] ATTRIBUTE_ALIGN(32) = {0};
static fstats g_isfsFileStats ATTRIBUTE_ALIGN(32) = {0};

#ifdef BACKUP_U8_ARCHIVE
static bool g_sdCardMounted = false;
#endif  /* BACKUP_U8_ARCHIVE */

/* Function prototypes. */

static u32 utilsButtonsDownAll(void);
static u32 utilsButtonsHeldAll(void);

void *utilsAllocateMemory(size_t size)
{
    void *ptr = NULL;
    size_t aligned_size = ALIGN_UP(size, 64);

    ptr = memalign(64, aligned_size);
    if (ptr) memset(ptr, 0, aligned_size);

    return ptr;
}

__attribute__((format(printf, 2, 3))) void utilsPrintErrorMessage(const char *func_name, const char *fmt, ...)
{
    va_list args;

    printf("%s: ", func_name);

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    printf("\n");
}

bool utilsIsWiiU(void)
{
    s32 ret = 0;
    u32 x = 0;

    ret = ES_GetTitleContentsCount(BC_NAND_TID, &x);
    if (ret < 0 || x == 0) return false;

    return true;
}

void utilsReboot(void)
{
    if (*(u32*)0x80001800) exit(0);
    SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
}

void utilsInitPads(void)
{
    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR);
}

u32 utilsGetInput(int type)
{
    if (type < UtilsInputType_Down || type > UtilsInputType_Held) return 0;

    VIDEO_WaitVSync();

    if (WPAD_ScanPads() <= WPAD_ERR_NONE) return 0;

    u32 pressed = (type == UtilsInputType_Down ? utilsButtonsDownAll() : utilsButtonsHeldAll());

    /* Convert Classic Controller values to WiiMote values. */
    if (pressed & WPAD_CLASSIC_BUTTON_ZR) pressed |= WPAD_BUTTON_PLUS;
    if (pressed & WPAD_CLASSIC_BUTTON_ZL) pressed |= WPAD_BUTTON_MINUS;

    if (pressed & WPAD_CLASSIC_BUTTON_PLUS) pressed |= WPAD_BUTTON_PLUS;
    if (pressed & WPAD_CLASSIC_BUTTON_MINUS) pressed |= WPAD_BUTTON_MINUS;

    if (pressed & WPAD_CLASSIC_BUTTON_A) pressed |= WPAD_BUTTON_A;
    if (pressed & WPAD_CLASSIC_BUTTON_B) pressed |= WPAD_BUTTON_B;
    if (pressed & WPAD_CLASSIC_BUTTON_X) pressed |= WPAD_BUTTON_2;
    if (pressed & WPAD_CLASSIC_BUTTON_Y) pressed |= WPAD_BUTTON_1;
    if (pressed & WPAD_CLASSIC_BUTTON_HOME) pressed |= WPAD_BUTTON_HOME;

    if (pressed & WPAD_CLASSIC_BUTTON_UP) pressed |= WPAD_BUTTON_UP;
    if (pressed & WPAD_CLASSIC_BUTTON_DOWN) pressed |= WPAD_BUTTON_DOWN;
    if (pressed & WPAD_CLASSIC_BUTTON_LEFT) pressed |= WPAD_BUTTON_LEFT;
    if (pressed & WPAD_CLASSIC_BUTTON_RIGHT) pressed |= WPAD_BUTTON_RIGHT;

    return pressed;
}

void utilsInitConsole(bool vwii)
{
    s32 x = 0, y = 0, w = 0, h = 0;

    /* Initialise the video system. */
    VIDEO_Init();

    /* Obtain the preferred video mode from the system. */
    /* This will correspond to the settings in the Wii menu. */
    g_rmode = VIDEO_GetPreferredMode(NULL);

    /* Set proper aspect ratio. */
    g_rmode->viWidth = 672;
    if (vwii)
    {
        /* Poke DMCU to turn off pillarboxing. */
        write32(0xd8006a0, (CONF_GetAspectRatio() == CONF_ASPECT_16_9 ? 0x30000004 : 0x10000002));
        mask32(0xd8006a8, 0, 2);
    }

    /* Set proper origin coordinates according to the video mode used. */
    if (g_rmode == &TVPal576IntDfScale || g_rmode == &TVPal576ProgScale)
    {
        g_rmode->viXOrigin = ((VI_MAX_WIDTH_PAL - g_rmode->viWidth) / 2);
        g_rmode->viYOrigin = ((VI_MAX_HEIGHT_PAL - g_rmode->viHeight) / 2);
    } else {
        g_rmode->viXOrigin = ((VI_MAX_WIDTH_NTSC - g_rmode->viWidth) / 2);
        g_rmode->viYOrigin = ((VI_MAX_HEIGHT_NTSC - g_rmode->viHeight) / 2);
    }

    /* Blackout display. */
    VIDEO_SetBlack(true);

    /* Set up the video registers with the chosen mode. */
    VIDEO_Configure(g_rmode);

    /* Flush the video register changes to the hardware. */
    VIDEO_Flush();

    /* Wait for video setup to complete. */
    VIDEO_WaitVSync();

    /* Allocate memory for the display in the uncached region. */
    g_xfb = SYS_AllocateFramebuffer(g_rmode);
    DCInvalidateRange(g_xfb, VIDEO_GetFrameBufferSize(g_rmode));
    g_xfb = MEM_K0_TO_K1(g_xfb);

    /* Clear the garbage around the edges. */
    VIDEO_ClearFrameBuffer(g_rmode, g_xfb, COLOR_BLACK);

    /* Tell the video hardware where our display memory is. */
    VIDEO_SetNextFramebuffer(g_xfb);

    /* Make the display visible. */
    VIDEO_SetBlack(false);

    /* Flush the video register changes to the hardware. */
    VIDEO_Flush();

    /* Wait for video setup to complete. */
    for(u8 i = 0; i < 4; i++) VIDEO_WaitVSync();

    /* Set console parameters. */
    x = 24;
    y = 32;
    w = (g_rmode->fbWidth - 32);
    h = (g_rmode->efbHeight - 48);

    /* Initialize the console - CON_InitEx works after VIDEO_ calls. */
    CON_InitEx(g_rmode, x, y, w, h);

    /* Set foreground color to white and reset all display attributes. */
    printf("\x1b[%u;%um", 37, false);
}

void utilsPrintHeadline(void)
{
    char ios_info[64] = {0};
    s32 rows = 0, cols = 0;

    utilsClearScreen();

    CON_GetMetrics(&cols, &rows);

    printf(APP_NAME " v" APP_VERSION ".");

    sprintf(ios_info, "IOS%d (v%d)", IOS_GetVersion(), IOS_GetRevision());
    printf("\x1b[%d;%dH", 0, cols - strlen(ios_info) - 1);
    printf(ios_info);

    printf("\nBuilt on " __DATE__ " - " __TIME__ ".\n");
    printf("Made by DarkMatterCore.\n\n");
}

signed_blob *utilsGetSignedTMDFromTitle(u64 title_id, u32 *out_size)
{
    if (!out_size) return NULL;

    s32 ret = 0;
    signed_blob *stmd = NULL;
    bool success = false;

    g_tmdTitleId = title_id;

    ret = ES_GetStoredTMDSize(g_tmdTitleId, &g_tmdSize);
    if (ret < 0)
    {
        ERROR_MSG("ES_GetStoredTMDSize failed! (%d) (TID %08X-%08X).", ret, TITLE_UPPER(g_tmdTitleId), TITLE_LOWER(g_tmdTitleId));
        return NULL;
    }

    stmd = (signed_blob*)utilsAllocateMemory(g_tmdSize);
    if (!stmd)
    {
        ERROR_MSG("Failed to allocate memory for TMD! (TID %08X-%08X).", TITLE_UPPER(g_tmdTitleId), TITLE_LOWER(g_tmdTitleId));
        return NULL;
    }

    ret = ES_GetStoredTMD(g_tmdTitleId, stmd, g_tmdSize);
    if (ret < 0)
    {
        ERROR_MSG("ES_GetStoredTMD failed! (%d) (TID %08X-%08X).", ret, TITLE_UPPER(g_tmdTitleId), TITLE_LOWER(g_tmdTitleId));
        goto out;
    }

    if (!IS_VALID_SIGNATURE(stmd))
    {
        ERROR_MSG("Invalid TMD signature! (TID %08X-%08X).", TITLE_UPPER(g_tmdTitleId), TITLE_LOWER(g_tmdTitleId));
        goto out;
    }

    *out_size = g_tmdSize;
    success = true;

out:
    if (!success && stmd)
    {
        free(stmd);
        stmd = NULL;
    }

    return stmd;
}

void *utilsReadFileFromIsfs(const char *path, u32 *out_size)
{
    if (!path || !*path || !out_size) return NULL;

    s32 ret = 0;
    u8 *buf = NULL;
    bool success = false;

    snprintf(g_isfsFilePath, ISFS_MAXPATH, path);

    g_isfsFd = ISFS_Open(g_isfsFilePath, ISFS_OPEN_READ);
    if (g_isfsFd < 0)
    {
        ERROR_MSG("ISFS_Open(\"%s\") failed! (%d).", g_isfsFilePath, g_isfsFd);
        return NULL;
    }

    ret = ISFS_GetFileStats(g_isfsFd, &g_isfsFileStats);
    if (ret < 0)
    {
        ERROR_MSG("ISFS_GetFileStats(\"%s\") failed! (%d).", g_isfsFilePath, ret);
        goto out;
    }

    if (!g_isfsFileStats.file_length)
    {
        ERROR_MSG("\"%s\" is empty!", g_isfsFilePath);
        goto out;
    }

    buf = (u8*)utilsAllocateMemory(g_isfsFileStats.file_length);
    if (!buf)
    {
        ERROR_MSG("Failed to allocate memory for \"%s\"!", g_isfsFilePath);
        goto out;
    }

    ret = ISFS_Read(g_isfsFd, buf, g_isfsFileStats.file_length);
    if (ret < 0)
    {
        ERROR_MSG("ISFS_Read(\"%s\") failed! (%d).", g_isfsFilePath, ret);
        goto out;
    }

    *out_size = g_isfsFileStats.file_length;
    success = true;

out:
    if (!success && buf)
    {
        free(buf);
        buf = NULL;
    }

    ISFS_Close(g_isfsFd);
    g_isfsFd = 0;

    return (void*)buf;
}

bool utilsWriteFileToIsfs(const char *path, void *buf, u32 size)
{
    if (!path || !*path || !buf || !size) return false;

    s32 ret = 0;
    bool success = false;

    snprintf(g_isfsFilePath, ISFS_MAXPATH, path);

    g_isfsFd = ISFS_Open(g_isfsFilePath, ISFS_OPEN_WRITE);
    if (g_isfsFd < 0)
    {
        ERROR_MSG("ISFS_Open(\"%s\") failed! (%d).", g_isfsFilePath, g_isfsFd);
        return false;
    }

    ret = ISFS_Write(g_isfsFd, buf, size);
    if (ret < 0)
    {
        ERROR_MSG("ISFS_Write(\"%s\") failed! (%d).", g_isfsFilePath, ret);
        goto out;
    }

    success = true;

out:
    ISFS_Close(g_isfsFd);
    g_isfsFd = 0;

    return success;
}

#ifdef BACKUP_U8_ARCHIVE
bool utilsMountSdCard(void)
{
    if (g_sdCardMounted) return true;
    g_sdCardMounted = fatMountSimple("sd", &__io_wiisd);
    return g_sdCardMounted;
}

void utilsUnmountSdCard(void)
{
    if (!g_sdCardMounted) return;
    fatUnmount("sd");
    __io_wiisd.shutdown();
    g_sdCardMounted = false;
}

bool utilsGetFileSystemStatsByPath(const char *path, u64 *out_total, u64 *out_free)
{
    char *name_end = NULL, stat_path[32] = {0};
    struct statvfs info = {0};
    int ret = -1;

    if (!path || !*path || !(name_end = strchr(path, ':')) || *(name_end + 1) != '/' || (!out_total && !out_free))
    {
        ERROR_MSG("Invalid parameters!");
        return false;
    }

    name_end += 2;
    sprintf(stat_path, "%.*s", (int)(name_end - path), path);

    if ((ret = statvfs(stat_path, &info)) != 0)
    {
        ERROR_MSG("statvfs(\"%s\") failed! (%d, %d).", path, ret, errno);
        return false;
    }

    if (out_total) *out_total = ((u64)info.f_blocks * (u64)info.f_frsize);
    if (out_free) *out_free = ((u64)info.f_bfree * (u64)info.f_frsize);

    return true;
}

void *utilsReadFileFromMountedDevice(const char *path, u32 *out_size)
{
    if (!path || !*path || !out_size) return NULL;

    FILE *fd = NULL;
    size_t filesize = 0, res = 0;
    u8 *buf = NULL;
    bool success = false;

    fd = fopen(path, "rb");
    if (!fd)
    {
        ERROR_MSG("fopen(\"%s\") failed! (%d).", path, errno);
        return NULL;
    }

    fseek(fd, 0, SEEK_END);
    filesize = (u32)ftell(fd);
    rewind(fd);

    if (!filesize)
    {
        ERROR_MSG("\"%s\" is empty!", path);
        goto out;
    }

    buf = (u8*)utilsAllocateMemory(filesize);
    if (!buf)
    {
        ERROR_MSG("Failed to allocate memory for \"%s\"!", path);
        goto out;
    }

    res = fread(buf, 1, filesize, fd);
    if (res != filesize)
    {
        ERROR_MSG("fread(\"%s\") failed! (%d). Read 0x%X, expected 0x%X.", path, errno, res, filesize);
        goto out;
    }

    *out_size = filesize;
    success = true;

out:
    if (!success && buf)
    {
        free(buf);
        buf = NULL;
    }

    fclose(fd);

    return (void*)buf;
}

bool utilsWriteFileToMountedDevice(const char *path, void *buf, u32 size)
{
    if (!path || !*path || !buf || !size) return false;

    FILE *fd = NULL;
    size_t res = 0;
    bool success = false;
    u64 free_space = 0;

    if (!utilsGetFileSystemStatsByPath(path, NULL, &free_space))
    {
        ERROR_MSG("Failed to retrieve free FS space!");
        return false;
    }

    if (free_space < (u64)size)
    {
        ERROR_MSG("Not enough free space available! Required 0x%X, available 0x%llX.", size, free_space);
        return false;
    }

    fd = fopen(path, "wb");
    if (!fd)
    {
        ERROR_MSG("fopen(\"%s\") failed! (%d).", path, errno);
        return false;
    }

    res = fwrite(buf, 1, size, fd);
    if (res != size)
    {
        ERROR_MSG("fwrite(\"%s\") failed! (%d). Wrote 0x%X, expected 0x%X.", path, errno, res, size);
        goto out;
    }

    success = true;

out:
    fclose(fd);

    return success;
}
#endif  /* BACKUP_U8_ARCHIVE */

static u32 utilsButtonsDownAll(void)
{
    int chan = 0;
    u32 pressed = 0;

    for(chan = WPAD_CHAN_0; chan <= WPAD_CHAN_3; chan++) pressed |= WPAD_ButtonsDown(chan);

    return pressed;
}

static u32 utilsButtonsHeldAll(void)
{
    int chan = 0;
    u32 pressed = 0;

    for(chan = WPAD_CHAN_0; chan <= WPAD_CHAN_3; chan++) pressed |= WPAD_ButtonsHeld(chan);

    return pressed;
}
