# Star Trek Fleet Command - Community Mod

<p align="center">
  <img src="https://img.shields.io/badge/License-GPLv3-blue.svg" alt="License: GPLv3">
  <img src="https://img.shields.io/github/sponsors/netniv" alt="Sponsorship">
</p>

<p align="center">
   A community mod (patch) for PC and Mac that adds a couple of tweaks to the <b>Star Trek Fleet Command&#8482;</b> game
</p>

## Downloads / Releases

The STFC Community Mod is available on GitHub.

For the latest official release only, see [https://github.com/netniv/stfc-mod/releases/latest/]

For both official and alpha/beta releases, see [https://github.com/netniv/stfc-mod/releases/]

Note: GitHub may require you to login to see the downloads for pre-releases and you will need to expand the
      assets section to see the downloads.

Note: There is no difference between the versioned and unversioned zip files, they just allow people to store multiple
      versions rather than overwritting or having a random number appended by windows.

## Installation

### Windows

1. Download the `stfc-community-patch.zip` file from your chosen [github release](https://github.com/netniv/stfc-mod/releases/) and extract the `version.dll` file.

2. Open the game folder in Explorer.  The default folder for both the game and the settings file is
   `C:\Games\Star Trek Fleet Command\Star Trek Fleet Command\default\game`

   If this folder isn't present, or no log files are created when running the game
   see the [windows-specific](#windows-specific) section below.

3. Move the extracted `version.dll` file into this folder.

4. Run the game!  If all is well, and one does not already exist, the mod will
   create a `community_patch_settings.toml` file populated with the default
   values.

5. For first time users of the Community Mod, it recommended to utilise the
   [sample configuration file](example_community_patch_settings.toml), which can
   be saved to the game folder with the name `community_patch_settings.toml`.  This
   sample file contains additional comments that explain the available settings.

### Mac

1. Download the `stfc-community-patch-installer.dmg` file from your chosen
[github release](https://github.com/netniv/stfc-mod/releases/), then open it and drag and drop the launcher to your Applications folder.  This launcher will be used to start the game any time you
want to have the mod loaded.

2. Run the game from the launcher!  If all is well, the mod will create the settings folder
`~/Library/Preferences/com.stfcmod.startrekpatch` with a `community_patch_settings.toml` file
populated with the default values.  

   Note: By default, MacOS hides the `~/Library` folder in Finder, so if it isn't visible,
   see the [mac-specific](#mac-specific) section below for tips on opening it.

3. For first time users of the Community Mod, it recommended to utilise the
   [sample configuration file](example_community_patch_settings.toml), which can
   be saved to the settings folder with the name `community_patch_settings.toml`.  This
   sample file contains additional comments that explain the available settings.

Note that the STFC game itself is located in the
`~/Library/Application Support/Star Trek Fleet Command/Games/Star Trek Fleet Command/STFC/default/game` folder.  You should only need to access this folder if you need to view the `community_patch.log` file while troubleshooting a problem.

### Wine/Linux

The Windows version of the mod, and the game itself, run well under Linux using the Wine compatability layer.
If the game is installed using the
[STFC installer for the Lutris game manager](https://lutris.net/games/star-trek-fleet-command/), the mod can
be installed by following the [Windows](#windows) directions above and using the
`~/Games/star-trek-fleet-command/drive_c/Games/Star Trek Fleet Command/Star Trek Fleet Command/default/game`
directory for installation.

If the game is installed through other launchers or directly in an unmanaged WINEPREFIX, an additional
adjustment may be needed for it to load the mod  library correctly; see the
[wine/linux specific](#winelinux-specific) notes below.

## Configuration

IMPORTANT: Any desired configuration changes should __ONLY__ be made in the
`community_patch_settings.toml` file.

When the game is launched, this file will be read.  Any missing settings that have not been set  in the 
_.toml_ file, will have a default value applied.  

When starting the game, the configuration including default values are re-written to `community_patch_runtime.vars`.  You should NOT make any changes to this file.  This file is 
constantly rewritten when the game starts so any modifications to it are never used and will be 
lost.

If you have any problems with a setting, check for that setting in the _.vars_ file to verify 
that the parse value of that setting was correctly applied.  You may also need to check the
`community_patch.log` file to see if any errors were encountered while parsing the _.toml_ file.

## Problems?

The most common problems getting the DLL to work are:

1. Not installed in the correct location.  This must be `Game` folder where `prime.exe` also exists.

2. Windows is blocking the DLL.  Right-click the file and select Properties.  On the `General` tab
   there will be additional text at the bottom:

   ```console
   This file can from another
   computer and might be blocked to
   help protect this computer
   ```

   To the right of this, there will be a tick box called `Unblock`.  Tick the box and then click OK
   to unblock the file.

3. The configuration file has the wrong name (see above)

4. The configuration file is not being parsed as you expect which is normally because:

   - Your configuration isn't being parsed
   - The configuration option name is spelt wrong
   - The configuration option name is in the wrong section
   - The configuration option value is not a true or false

   You can verify your configuration by looking at `community_patch_runtime.vars` and/or the
   log file `community_patch.log`.

### Windows specific

On windows, sometimes the game is installed in a specific users profile folder.  This is suspected
to happen when it thinks more than one user attempted to install it, or if that user doesn't have
admin rights.

If this is the case, to find the correct game location for the `version.dll` file, do the following:

1) Open Task Manager while the game is running (press Ctrl-Alt-Delete)
2) Right-click `prime.exe` and select 'Expand'
3) Right-click `Star Trek Fleet Command` and select 'Open file location'

An Explorer window will open with the correct folder selected.

### Mac Specific

MacOS hides the `Library` folder in Finder by default. There are several ways to open it:

- Hold the `⌥ option` key (or `Alt` on a PC keyboard) in __Finder__ and click the __Go to__ menu item
and then click the __Library__ folder item
- Press `⌘ cmd`+`⇧ shift`+`G` in the __Finder__ for the Goto box and type in `~/Library`
- Use __Terminal__ and type: `open ~/Library`

### Wine/Linux Specific

To use the Windows version of the mod under Wine, the wine DLL override setting for `version.dll` _must_ be set to `n,b` or it will not be loaded.  If you have installed the game using the [STFC installer for the Lutris game manager](https://lutris.net/games/star-trek-fleet-command/) (recommended), this override will have already been defined in the runner configuration.   Otherwise, it can be set in the `winecfg.exe` Libraries tab or by setting the `WINEDLLOVERRIDES` enviroment variable to `version.dll=n,b` before launching the game.

For installation, generally follow the Windows directions above.  The only real change
will be the location of the game folder, which should be in the directory `drive_c/Games/Star Trek Fleet Command/Star Trek Fleet Command/default/game` relative the wine environment (WINEPREFIX) you're running it under.
