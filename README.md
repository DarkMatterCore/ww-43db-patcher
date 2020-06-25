# ww-43db-patcher
vWii WiiWare 43DB patcher.

This is a Wii homebrew application that patches the WiiWare aspect ratio database (43DB) from the vWii System Menu U8 archive to stub all of its entries, effectively enabling access to 16:9 aspect ratio in WiiWare titles where 4:3 is otherwise enforced.

A backup of the unpatched System Menu U8 archive content file is created at `sd:/ww-43db-patcher_bkp/<content_id>.app`. It is recommended to copy it to a safer location.

For obvious reasons, this only works under Wii U consoles. Big thanks to [InvoxiPlayGames](https://github.com/InvoxiPlayGames) for testing.

ww-43db-patcher is licensed under GPLv2.