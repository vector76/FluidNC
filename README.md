# What this?
This is a stopgap, temporary experimental build of FluidNC that adds the ability to chain files together using a macro hack.  If you don't know what this is or why you need it, you're in the wrong place and it's not for you.

This version adds a macro `on_idle` that executes when a job finishes and transitions from Running to Idle.  It is visible in the WebUI and on the console via `$/macros/on_idle` but it is mainly intended to be assigned from within gcode files on the SD card to launch files in sequence.

Do not try use this from streaming senders.

# IMPORTANT RULES!

## Do not ask for help on Discord
Even if you think the problem is unrelated to file chaining, do not ask about *any* problems if you are using this build.  Seriously.

Reload the standard FluidNC firmware and see if your issue persists.  Then you can honestly ask about the official FluidNC firmware.

You may open an issue on https://github.com/vector76/FluidNC/issues if you think it is related to the file chaining.

## This version will go away eventually
If you have saved a copy of the firmware you may use it indefinitely, but when this feature becomes obsolete, this branch will be removed and you will be stranded.  You will need to change all your gcode files to use the updated, better method.

When this occurs, you may not complain or ask for support for "the old method".  This is a stopgap temporary build only.

This firmware will probably not get any updates.

# How to use

### 1. Download and install the firmware.bin file
Download this binary:
https://github.com/vector76/FluidNC/raw/on_idle_binary/firmware.bin

### 2. At the top of every gcode file, add this:
```
$/macros/on_idle=
```
Make sure you have the `=` at the end and nothing after that.  This clears the next file in the chain (if any) so it will only go into an infinite loop when you want, and if you cancel a job it won't launch unintentionally.

### 3. At the end of your gcode file, add the chaining for the next file like this:
```
$/macros/on_idle=$sd/run=<your file>
```
For example
```
$/macros/on_idle=$sd/run=circle200b.gcode
```
By adding it at the end of the file, it won't falsely be triggered by spurious idle transitions.

### 4. Watch for special hazards
Do not use G4 as the last "G" command in your file.  (There is a bug that causes this to be treated as an idle transition.)

If you wish to delay at the end before the next file, you must add a movement after the G4.
