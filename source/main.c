/*
 * main.c
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

#include "utils.h"
#include "ardb.h"

#include <runtimeiospatch.h>

extern void __exception_setreload(int t);

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    int ret = 0;
    bool vwii = utilsIsWiiU(), bail = false;

    /* Set reload time to 10 seconds in case an exception is triggered. */
    __exception_setreload(10);

    /* Initialize video output and controllers. */
    utilsInitConsole(vwii);
    utilsInitPads();

    /* Print headline. */
    utilsPrintHeadline();

    /* Check if we're running under vWii (Wii U). */
    if (!vwii)
    {
        printf("Error: not running on a Wii U!");
        ret = -1;
        goto out;
    }

    /* Check if we have full hardware access (HW_AHBPROT flag disabled). */
    if (!AHBPROT_DISABLED)
    {
        printf("The HW_AHBPROT hardware register is not disabled.\n");
        printf("Maybe you didn't load the application from a loader\n");
        printf("capable of passing arguments (you should use HBC\n");
        printf("1.1.0 or later). Or, perhaps, you don't have the\n");
        printf("\"<ahb_access/>\" element in the meta.xml file, which\n");
        printf("is very important.\n\n");
        printf("This application can't do its job without full\n");
        printf("hardware access rights.");
        ret = -2;
        goto out;
    }

    /* Apply runtime IOS patches. */
    printf("Applying runtime IOS patches, please wait... ");

    ret = IosPatch_RUNTIME(true, false, false, false);
    if (ret <= 0)
    {
        printf("FAILED!");
        ret = -3;
        goto out;
    }

    printf("OK!\n");

    /* Initialize NAND filesystem driver. */
    printf("Initializing NAND FS driver... ");

    ret = ISFS_Initialize();
    if (ret < 0)
    {
        printf("FAILED!");
        ret = -4;
        goto out;
    }

    printf("OK!\n");

#ifdef BACKUP_U8_ARCHIVE
    /* Mount SD card. */
    printf("Mounting SD card... ");

    if (!utilsMountSdCard())
    {
        printf("FAILED!");
        ret = -5;
        goto out;
    }

    printf("OK!\n\n");
#endif  /* BACKUP_U8_ARCHIVE */

    printf("Press 1/Y to patch the WiiWare aspect ratio database (WW 43DB).\n");
    printf("Press 2/X to *only* patch WC24 channel entries within the WW 43DB.\n");
#ifdef BACKUP_U8_ARCHIVE
    printf("Press - to restore a backup of the System Menu U8 archive.\n");
#endif  /* BACKUP_U8_ARCHIVE */
    printf("Press HOME to exit.\n\n");

    fflush(stdout);

    while(true)
    {
        u32 pressed = utilsGetInput(UtilsInputType_Down);
        bool wc24_only = (pressed == WPAD_BUTTON_2);

        if (pressed == WPAD_BUTTON_1 || wc24_only)
        {
            /* Patch WiiWare aspect ratio database. */
            printf("Patching WiiWare aspect ratio database (%s entries)...\n\n", wc24_only ? "WC24" : "all");

            if (!ardbPatchDatabaseFromSystemMenuArchive(wc24_only ? AspectRatioDatabaseType_WiiWareWC24Only : AspectRatioDatabaseType_WiiWare))
            {
                ret = -6;
                goto out;
            }

            break;
        } else
#ifdef BACKUP_U8_ARCHIVE
        if (pressed == WPAD_BUTTON_MINUS)
        {
            /* Restore System Menu U8 archive backup. */
            printf("Restoring System Menu U8 archive...\n\n");

            if (!ardbRestoreSystemMenuArchive())
            {
                ret = -7;
                goto out;
            }

            break;
        } else
#endif  /* BACKUP_U8_ARCHIVE */
        if (pressed == WPAD_BUTTON_HOME)
        {
            /* Exit. */
            bail = true;
            break;
        }
    }

    if (!bail) printf("Process completed. Press any button to exit.");

out:
#ifdef BACKUP_U8_ARCHIVE
    utilsUnmountSdCard();
#endif  /* BACKUP_U8_ARCHIVE */

    ISFS_Deinitialize();

    if (ret != 0) printf("\n\nProcess cannot continue. Press any button to exit.");

    if (!bail) utilsWaitForButtonPress();

    utilsReboot();

    return ret;
}
