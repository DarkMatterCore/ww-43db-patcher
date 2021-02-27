# ww-43db-patcher
vWii WiiWare 43DB patcher.

This is a Wii homebrew application that patches the WiiWare aspect ratio database (43DB) from the vWii System Menu U8 archive to stub all of its entries, effectively enabling access to 16:9 aspect ratio in WiiWare titles where 4:3 is otherwise enforced. The System Menu TMD isn't modified in this process.

A backup of the unpatched System Menu U8 archive content file is created at `sd:/ww-43db-patcher_bkp/<content_id>.app`. It is recommended to copy it to a safer location. The application is also capable of restoring such backup on its own, as long as it is available in the SD card.

If the U8 archive has been modified in some kind of way and its hash no longer matches the one from the System Menu TMD, backup generation and restoring features won't work.

For obvious reasons, this only works under Wii U consoles. Big thanks to [InvoxiPlayGames](https://github.com/InvoxiPlayGames) for both testing and providing the icon!

License
--------------

ww-43db-patcher is licensed under GPLv2.

Changelog
--------------

**v0.2:**

* Now capable of restoring a previously generated System Menu U8 archive backup.
* System Menu U8 archive content hash is now verified before attempting to save a backup or restore it.
* Moved all SD card I/O in ardb.c to utils.c.
* Minor optimizations.

**v0.1:**

* Initial release.
