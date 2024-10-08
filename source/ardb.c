/*
 * ardb.c
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

#include "utils.h"
#include "ardb.h"
#include "u8.h"
#include "sha1.h"

static const char *g_ardbArchivePaths[AspectRatioDatabaseType_Count] = {
    "/titlelist/discdb.bin",
    "/titlelist/vcadb.bin",
    "/titlelist/wwdb.bin"
};

bool ardbPatchDatabaseFromSystemMenuArchive(u8 type, const u32 *entries, const u32 entry_count)
{
    if (type >= AspectRatioDatabaseType_Count)
    {
        ERROR_MSG("Invalid aspect ratio database type value!");
        return false;
    }

    if (!entries || !entry_count)
    {
        ERROR_MSG("Invalid patch entries array / count!");
        return false;
    }

    signed_blob *sysmenu_stmd = NULL;
    u32 sysmenu_stmd_size = 0;

    tmd *sysmenu_tmd = NULL;
    tmd_content *sysmenu_archive_content = NULL;

    char content_path[ISFS_MAXPATH] = {0};
    u8 *sysmenu_archive_content_data = NULL;
    u32 sysmenu_archive_content_size = 0;

    U8Context u8_ctx = {0};
    u32 u8_node_idx = 0;

    u8 *ardb_data = NULL;
    u32 ardb_data_size = 0, ardb_orig_entry_count = 0;
    AspectRatioDatabase *ardb = NULL;

    bool success = false;

#ifdef BACKUP_U8_ARCHIVE
    char backup_path[ISFS_MAXPATH] = {0};
    sha1 sysmenu_archive_content_hash = {0};
    bool hash_match = false, backup_created = false;
#endif  /* BACKUP_U8_ARCHIVE */

    /* Get System Menu TMD. */
    sysmenu_stmd = utilsGetSignedTMDFromTitle(SYSTEM_MENU_TID, &sysmenu_stmd_size);
    if (!sysmenu_stmd)
    {
        ERROR_MSG("Error retrieving System Menu TMD!");
        goto out;
    }

    sysmenu_tmd = utilsGetTMDFromSignedBlob(sysmenu_stmd);

    /* Look for the biggest content record (U8 archive with resources). */
    sysmenu_archive_content = &(sysmenu_tmd->contents[0]);
    for(u16 i = 0; i < sysmenu_tmd->num_contents; i++)
    {
        if (sysmenu_tmd->contents[i].size > sysmenu_archive_content->size) sysmenu_archive_content = &(sysmenu_tmd->contents[i]);
    }

    /* Generate U8 archive content path and read the whole content file. */
    sprintf(content_path, "/title/%08x/%08x/content/%08x.app", TITLE_UPPER(SYSTEM_MENU_TID), TITLE_LOWER(SYSTEM_MENU_TID), sysmenu_archive_content->cid);
    sysmenu_archive_content_data = (u8*)utilsReadFileFromIsfs(content_path, &sysmenu_archive_content_size);
    if (!sysmenu_archive_content_data)
    {
        ERROR_MSG("Failed to read System Menu U8 archive content data!");
        goto out;
    }

#ifdef BACKUP_U8_ARCHIVE
    /* Calculate U8 archive content hash. */
    sha1CalculateHash(sysmenu_archive_content_data, sysmenu_archive_content_size, sysmenu_archive_content_hash);

    /* Compare hashes. */
    hash_match = (memcmp(sysmenu_archive_content->hash, sysmenu_archive_content_hash, SHA1_HASH_SIZE) == 0);
    if (hash_match)
    {
        /* Create output directory. */
        sprintf(backup_path, "sd:/" APP_TITLE "_bkp");
        mkdir(backup_path, 0777);

        /* Generate backup content path. */
        strcat(backup_path, strrchr(content_path, '/'));

        /* Write backup content file. */
        backup_created = utilsWriteFileToMountedDevice(backup_path, sysmenu_archive_content_data, sysmenu_archive_content_size);
        if (!backup_created)
        {
            ERROR_MSG("Failed to write U8 archive backup!");
            goto out;
        }

        printf("Saved System Menu U8 archive backup to \"%s\".\nPlease copy it to a safe location.\n\n", backup_path);
    } else {
        printf("U8 archive content hash mismatch! Skipping backup generation.\n\n");
    }
#endif  /* BACKUP_U8_ARCHIVE */

    /* Initialize U8 context. */
    if (!u8ContextInit(sysmenu_archive_content_data, sysmenu_archive_content_size, &u8_ctx))
    {
        ERROR_MSG("Failed to initialize System Menu U8 archive context!");
        goto out;
    }

    /* Get U8 node for the aspect ratio database path. */
    if (!u8GetFileNodeByPath(&u8_ctx, g_ardbArchivePaths[type], &u8_node_idx))
    {
        ERROR_MSG("Failed to retrieve U8 node for \"%s\"!", g_ardbArchivePaths[type]);
        goto out;
    }

    /* Read aspect ratio database data. */
    if (!(ardb_data = u8LoadFileData(&u8_ctx, u8_node_idx, &ardb_data_size)) || ardb_data_size < sizeof(AspectRatioDatabase))
    {
        ERROR_MSG("Failed to read \"%s\" contents from U8 archive!", g_ardbArchivePaths[type]);
        goto out;
    }

    /* Parse aspect ratio database. */
    ardb = (AspectRatioDatabase*)ardb_data;

    if (ardb->magic != ARDB_MAGIC)
    {
        ERROR_MSG("Invalid ARDB magic word for \"%s\": 0x%08X.", g_ardbArchivePaths[type], ardb->magic);
        goto out;
    }

    if (!ardb->entry_count || ardb_data_size < (sizeof(AspectRatioDatabase) + (sizeof(u32) * ardb->entry_count)))
    {
        ERROR_MSG("Invalid ARDB entry count for \"%s\": %u", g_ardbArchivePaths[type], ardb->entry_count);
        goto out;
    }

    ardb_orig_entry_count = ardb->entry_count;

    printf("Loaded \"%s\" (v%u, holding %u %s)", g_ardbArchivePaths[type], ardb->version, ardb->entry_count, (ardb->entry_count == 1 ? "entry" : "entries"));

#ifdef DISPLAY_ARDB_ENTRIES
    if (ardb->entry_count)
    {
        printf(":\n");

        for(u32 i = 0; i < ardb->entry_count; i++)
        {
            printf("%.*s", 3, (char*)&(ardb->entries[i]));
            if (i < (ardb->entry_count - 1)) printf(", ");
        }

        printf("\n\n");
    } else {
        printf(".\n\n");
    }
#else   /* DISPLAY_ARDB_ENTRIES */
    printf(".\n\n");
#endif  /* DISPLAY_ARDB_ENTRIES */

    fflush(stdout);

    /* Patch aspect ratio database. */
    for(u32 i = 0, j = 0; i < ardb->entry_count; i++, j++)
    {
        for(u32 k = 0; k < entry_count; k++)
        {
            u32 val = (ardb->entries[i] >> 8);
            if (val != entries[k]) continue;

            /* Jackpot. */
            printf("Removing 43DB entry #%u: %.*s. (0x%X).\n", j, 3, (char*)&val, val);
            fflush(stdout);

            if ((i + 1) < ardb->entry_count) memmove(&(ardb->entries[i]), &(ardb->entries[i + 1]), sizeof(u32) * (ardb->entry_count - i - 1));
            memset(&(ardb->entries[ardb->entry_count - 1]), 0, sizeof(u32));

            ardb->entry_count--;
            i--;

            break;
        }
    }

    if (ardb->entry_count == ardb_orig_entry_count)
    {
        ERROR_MSG("Unable to locate desired TIDs within \"%s\". No changes have been made.", g_ardbArchivePaths[type]);
        goto out;
    }

    printf("\n");
    fflush(stdout);

    /* Save modified aspect ratio database data to U8 archive buffer. */
    ardb_data_size = (sizeof(AspectRatioDatabase) + (sizeof(u32) * ardb->entry_count));

    if (!u8SaveFileData(&u8_ctx, u8_node_idx, ardb_data, ardb_data_size))
    {
        ERROR_MSG("Failed to save modified aspect ratio database data into U8 archive!");
        goto out;
    }

    /* Write modified U8 archive buffer to the NAND storage. */
    if (!utilsWriteFileToIsfs(content_path, sysmenu_archive_content_data, sysmenu_archive_content_size))
    {
        ERROR_MSG("Failed to write modified U8 archive to \"%s\"!", content_path);
        goto out;
    }

    /* Update output flag. */
    success = true;

out:
    if (ardb_data) free(ardb_data);

    u8ContextFree(&u8_ctx);

    if (sysmenu_archive_content_data) free(sysmenu_archive_content_data);

    if (sysmenu_stmd) free(sysmenu_stmd);

#ifdef BACKUP_U8_ARCHIVE
    if (hash_match && !backup_created)
    {
        sprintf(backup_path, "sd:/" APP_TITLE "_bkp");
        remove(backup_path);
    }
#endif  /* BACKUP_U8_ARCHIVE */

    return success;
}

#ifdef BACKUP_U8_ARCHIVE
bool ardbRestoreSystemMenuArchive(void)
{
    signed_blob *sysmenu_stmd = NULL;
    u32 sysmenu_stmd_size = 0;

    tmd *sysmenu_tmd = NULL;
    tmd_content *sysmenu_archive_content = NULL;

    char content_path[ISFS_MAXPATH] = {0};

    char backup_path[ISFS_MAXPATH] = {0};
    u8 *backup_content_data = NULL;
    u32 backup_content_size = 0;
    sha1 backup_content_hash = {0};

    bool success = false;

    /* Get System Menu TMD. */
    sysmenu_stmd = utilsGetSignedTMDFromTitle(SYSTEM_MENU_TID, &sysmenu_stmd_size);
    if (!sysmenu_stmd)
    {
        ERROR_MSG("Error retrieving System Menu TMD!");
        goto out;
    }

    sysmenu_tmd = utilsGetTMDFromSignedBlob(sysmenu_stmd);

    /* Look for the biggest content record (U8 archive with resources). */
    sysmenu_archive_content = &(sysmenu_tmd->contents[0]);
    for(u16 i = 0; i < sysmenu_tmd->num_contents; i++)
    {
        if (sysmenu_tmd->contents[i].size > sysmenu_archive_content->size) sysmenu_archive_content = &(sysmenu_tmd->contents[i]);
    }

    /* Generate U8 archive content path. */
    sprintf(content_path, "/title/%08x/%08x/content/%08x.app", TITLE_UPPER(SYSTEM_MENU_TID), TITLE_LOWER(SYSTEM_MENU_TID), sysmenu_archive_content->cid);

    /* Generate backup content path. */
    sprintf(backup_path, "sd:/" APP_TITLE "_bkp");
    strcat(backup_path, strrchr(content_path, '/'));

    /* Read whole backup content file. */
    backup_content_data = (u8*)utilsReadFileFromMountedDevice(backup_path, &backup_content_size);
    if (!backup_content_data)
    {
        ERROR_MSG("Failed to read System Menu U8 archive backup!");
        goto out;
    }

    /* Calculate U8 archive content hash. */
    sha1CalculateHash(backup_content_data, backup_content_size, backup_content_hash);

    /* Compare hashes. */
    if (memcmp(sysmenu_archive_content->hash, backup_content_hash, SHA1_HASH_SIZE) != 0)
    {
        ERROR_MSG("U8 archive content backup hash mismatch!");
        goto out;
    }

    /* Write U8 archive buffer to the NAND storage. */
    if (!utilsWriteFileToIsfs(content_path, backup_content_data, backup_content_size))
    {
        ERROR_MSG("Failed to write U8 archive backup to \"%s\"!", content_path);
        goto out;
    }

    /* Update output flag. */
    success = true;

out:
    if (backup_content_data) free(backup_content_data);

    if (sysmenu_stmd) free(sysmenu_stmd);

    return success;
}
#endif  /* BACKUP_U8_ARCHIVE */
