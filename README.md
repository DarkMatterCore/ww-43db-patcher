# ww-43db-patcher
vWii WiiWare 43DB patcher.

This is a Wii homebrew application that patches the WiiWare 4:3 aspect ratio database (43DB) within vWii's System Menu U8 archive to remove WiiConnect24-related channel entries (Everybody Votes Channel, Check Mii Out Channel) from it, effectively enabling access to a 16:9 aspect ratio. The System Menu TMD isn't modified in this process.

A backup of the unpatched System Menu U8 archive content file is created at `sd:/ww-43db-patcher_bkp/<content_id>.app`. It is recommended to copy it to a safer location. The application is also capable of restoring such backup on its own, as long as it is available in the SD card.

If the U8 archive has been modified in some kind of way and its hash no longer matches the one from the System Menu TMD, backup generation and restoring features won't work.

For obvious reasons, this only works under Wii U consoles.

License
--------------

ww-43db-patcher is licensed under GPLv2.

Acknowledgments
--------------

* [InvoxiPlayGames](https://github.com/InvoxiPlayGames) for both testing and providing the icon.
* [Ingunar](https://github.com/Ingunar), for helping with some extra tests.

Changelog
--------------

**v0.4:**

* Change ARDB patching behavior: instead of stubbing all title records to `ZZZ.`, the desired entries are simply removed from the target ARDB.
* Remove option to patch all entries from the WW ARDB.

**v0.3:**

* Fix borked error message output due to missing function attributes.
* Check free space on the inserted SD card before attempting to write a System Menu U8 archive backup.
* Migrate to hardware-based SHA-1 hash calculation.
* Add option to only patch most demanded WC24 channel entries (Everybody Votes Channel, Check Mii Out Channel).
* Display git branch and commit hash.
* Display build date in UTC format.
* Reset screen on user input to better accommodate for any possible error messages (except when the HOME button is pressed).
* Other minor fixes and improvements.

**v0.2:**

* Now capable of restoring a previously generated System Menu U8 archive backup.
* System Menu U8 archive content hash is now verified before attempting to save a backup or restore it.
* Moved all SD card I/O in ardb.c to utils.c.
* Minor optimizations.

**v0.1:**

* Initial release.
