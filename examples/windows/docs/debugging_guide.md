<!--
     Copyright 2026, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# How to setup Windows kernel debugging

This is a guide on how to setup a dual VM debugging rig. It is useful
because when Windows encounters a fatal problem, it will only give you
a generic error code/message, or none at all if the kernel have not booted
far enough.

The setup is as follows: libvmm with a
non-working Windows VM as the "debuggee VM". Then Windows on bare-QEMU
as the "debugger VM". Both VMs' COM1 port will be connected via TCP
so that the debugger and debuggee can communicate. This guide assumes you
know how to install Windows on QEMU.

## Debugger

Before starting the VM, we must tell the QEMU instance of the debugger
VM to act as a "serial server" on localhost. So that the debuggee VM
can later connect to this server and send/receive data, which will appear to
the debugger Windows on COM1. This is how WinDbg will talk to the debuggee VM.
Append this argument to your QEMU command:
`-serial tcp:127.0.0.1:4444,server=on,wait=off`. Make sure that this is
the first `-serial` argument so that the serial device lives on COM1.

Start the VM and install Windows 10 or 11. Go through the OOBE
to reach the desktop. Alternatively, use the automated process
as described in the higher level README.md.

Then install the latest version of WinDbg.

Connect to the debuggee VM with the following steps:
1. Start WinDbg
2. Press 'File'
3. Press 'Attach to kernel'
4. Make sure you are in the 'COM' tab.
5. The defaults (COM1 at baud 115200) will work.
6. Tick 'Break on connection'.
7. Press 'Ok'
8. You should see "Waiting to reconnect" on WinDbg's console.

The debugger VM is now ready. When the debuggee VM boots up, WinDbg will connect
automatically and give you a kernel debugging environment with Microsoft's
public symbol files.

I recommend that you turn off Windows Update, so that the binaries doesn't get changed
under your feet. This is useful for debugging as detailed later.

My typical workflow is that I just press 'Go' until the kernel crash. Then
start printing out the state of various subsystems in the Windows kernel.
The debugger also give you a stacktrace so you can look at the problematic
kernel function under Ghidra.

## Debuggee

Make a clone of the debugger image, the clone will now be the debuggee.

Boot the debuggee image on QEMU, open a command prompt with admin
privilege, then run these commands:

```
bcdedit /debug on
bcdedit /dbgsettings serial debugport:1 baudrate:115200
bcdedit /bootdebug {current} on
bcdedit /bootdebug {bootmgr} on
```

If you want to debug Windows PE (the recovery environment) as well
then run these commands:

```
bcdedit /enum {current}
# take GUID of "recoverysequence"
bcdedit /bootdebug {REPLACE_WITH_GUID} on
bcdedit /debug {REPLACE_WITH_GUID} on
```

Windows will now wait for the debugger to connect on COM1 at boot.
Shut down the VM cleanly, convert the image into libvmm format then
boot it on libvmm. Make sure that the VMM passed through COM1 to the VM.
You will need to append this argument to the debuggee VM's QEMU instance
to allow the kernel to talk to WinDbg on the debugger VM:
`-serial tcp:127.0.0.1:4444`. Make sure that this is
the first `-serial` argument so that the serial device lives on COM1.

When the OVMF finishes loading Windows and jumps to it, WinDbg will
connect automatically.

### EMS console
If you want to use the Emergency Management Console on COM1 after you
got Windows working, run the
following commands in an elevated command prompt:

```
bcdedit /emssettings EMSPORT:1 EMSBAUDRATE:115200
bcdedit /ems {current} on
```

## Kernel decompilation

Encountered a Windows crash, but not sure why? You can decompile the
problematic binary with Microsoft public symbol files. For example,
if the problematic function is `nt!something`, then it is inside the
Windows NT kernel. You will need a matching copy of `ntoskrnl.exe` by
either mounting the debuggee image and copying it out. Or send it to
yourself from the debugger VM since the image is the same!

Steps:
1. Download Ghidra and follow their guide to get it working on your machine.
2. Create a project with a name to your liking.
3. You will now be dropped into the main menu.
4. Import the crashing binary by pressing 'File' -> 'Import File'.
5. The binary will now be an entry under 'Active Project'.
6. Double-click the binary to open it in the CodeBrowser.
7. You will get a prompt "XYZ has not been analysed. Would you like to analyse it now?".
   Press No, because we need to load Microsoft's symbol files first.
8. On the toolbar, click 'File' -> 'Load PDB File'.
9. In the pop up click 'Advanced' -> 'Config'.
10. Click the plus icon.
11. Select 'msdl.microsoft.com...'.
12. Click 'Ok'.
13. Ghidra will now recognise which symbol file it need to download.
14. Click 'Load'.
15. On the toolbar, click 'Analysis' -> 'Auto Analyse...'.
16. Feel free to play with the options, the default worked well enough for me.
17. Click `Analyse` then let it run for a few minutes. When it finish, it will
    prompt you to jump to the entrypoint.