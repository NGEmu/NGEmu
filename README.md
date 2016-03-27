# NGEmu
NGEmu is a HLE N-Gage emulator written in C++.

IRC channel: #NGEmu on Freenode

## Status
**Development halted** until we can obtain the **N-Gage specific SDK**.  
If you've got the SDK, parts of it or any leads for obtaining it, please contact tambre either on our IRC channel (#NGEmu on Freenode) or through ![](https://i.imgur.com/bPYEQsM.png)

**But why?**  
Since a NGEmu is a HLE emulator, we depend on ordinals for functions being correct. We currently have been using IDs generated from the 1st Edition S60 SDK, but these don't match up with what the game actually passes as arguments. To avoid incorrect implementation and problems down the road, the progress is halted for now.

Other than that, some basic game code runs.

## System Requirements
* Modern 64-bit CPU
* Vulkan compatible graphics card
* 64-bit OS

## Compiling
NGEmu can be currently only compiled using Visual Studio 2015 on a Windows computer. Linux support is planned for the future.
