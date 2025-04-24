Idlers tavern : https://discord.gg/FD2cYKBq5E  << Join for latest updates OT modules services c++ codes lua scripts and engines.
=============
Prerequisites:

https://www.techpowerup.com/download/visual-c-redistributable-runtime-package-all-in-one/
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^



[Features]

There is EXE FILE ALREADY IN REPOSITORY YOU DO NOT NEED TO DOWNLOAD ANY BUILD JUST OPEN VIA EDITOR_x64 
to use collections copy collections.xml from 10.98 folder to any other version you are using. ensure the correct menubar.xml file is used! i use the one in vcproj/x64/release/data

Find Item Window
[added various flags such as isHelmet hasLight]
[search by Item range]
[ignore range]

Replace Items / Replace Selection
Massive overhaul adding whole border and wall lists!
Allow brush input

Minimap 
various performance optimizations
LOD optimization ZOOM x8+ renders only ground floor
Export optmization ignore empty tiles

Collections
copy selected part of map and add to doodads via right click allows for speedmapping corridors and islands borders etc

Fill
fill empty space
fill connected borders ( empty borders that have no water tile under them or ground tile it will search all connected borders)

Remove duplicates

Browse field 
added browsefield expansion allowing to move items on stackpos

Remove unreachable Tiles
expanded for custom clients

Find Item Action / Unique Id 
find range or specific item action unique ID

goto position
take various formats of the position xyz style

Hotkey Configuration
added hotkey config menu

Right click select raw brush instantly
option in preferences

Borderize batching
if u borderized large maps the editor would crash it now does it by batches of 500 tiles

Town index
change selected town index id


Automagic
layer carpet brushes
same ground type border


Dark mode
& colored mode


Live Server(Experimental only works in debug mode after ignoring first breakpoint 3-4times then u can draw fine
lack of synchronization
lack of map chunk batching

Autosave option feature
u can save map x seconds e.g 300 seconds in /data/autosave will save your backup maps!


Validate grounds
validate all ground tiles on map remove duplicates and wrongly stacked ground tiles


version 6.0.0 - 6.1.0 [not public]
Grid view final
allows for displaying raw palette in grid style with resizable palette window
Added a "Load NPCs Folder" button to the CreaturePalettePanel
Added a Load Monsters Npc folder to Creaturepalette panel
Generate House - Creates house
Map properties fetch otbm verison based on client verison
Detached view - ever wanted to edit 5 different parts of map on same time? 
And many more.


I often work till late I do not have the time to make commits im the guy that does the job since its open source you guys go fix commits atleast it will show anybody proofread it before running this spaghetti code that been fueled by pure hatred.


![image](https://github.com/user-attachments/assets/fa261227-5d88-4d00-9c60-0f48e393adc5)


![image](https://github.com/user-attachments/assets/44c2defe-7cb3-4998-b371-15b1387a4b7f)





Rookgaard Independence
https://discord.gg/CavZvnvRey

TIBIA TRADE:
https://discord.gg/BtrYU3tn7a

ADVERTISE YOUR OT SERVER WITH Idler_pl ON DISCORD.

This is a map editor for game servers that derivied from [OpenTibia](https://github.com/opentibia/server) server project.

It is a fork of a [Map Editor](https://github.com/hampusborgos/rme) created by [Remere](https://github.com/hampusborgos).

You can find an engine compatible with OTBM format at [OTAcademy](https://github.com/OTAcademy), [OTLand](https://github.com/OTLand), [OpenTibiaBR](https://github.com/OpenTibiaBR) or other OT communities.

Visit [OTARMEIE discord](https://discord.gg/FD2cYKBq5E) if you are looking for support or updates.

I want to contribute
====================

Yeah right. 

Bugs
======

Have you found a bug? Me too!

Other Applications
==========
XD
Download
========

Idlers Tavern discord has latest builds
https://discord.gg/FD2cYKBq5E


Compiling
=========
Required libraries:
* wxWidgets >= 3.0
* Boost >= 1.55.0

### VCPKG libraries:
* 32-bit : `vcpkg install wxwidgets freeglut asio nlohmann-json fmt libarchive boost-spirit`
* 64-bit : `vcpkg install --triplet x64-windows wxwidgets freeglut asio nlohmann-json fmt libarchive boost-spirit`

[Compile on Windows](https://github.com/hjnilsson/rme/wiki/Compiling-on-Windows)

[Compile on Ubuntu](https://github.com/hjnilsson/rme/wiki/Compiling-on-Ubuntu)

[Compile on Arch Linux](https://github.com/hjnilsson/rme/wiki/Compiling-on-Arch-Linux)

[Compile on macOS](https://github.com/hjnilsson/rme/wiki/Compiling-on-macOS)

Extras:
https://idler.live/

![image](https://github.com/user-attachments/assets/aa00772e-8067-4b96-88b4-97dd0bbbb0f9)


download exe dll
[Release (1).zip](https://github.com/user-attachments/files/19621775/Release.1.zip)

and put in OTARMEIE FOLDER of this repo 

