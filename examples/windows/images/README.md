<!--
     Copyright 2026, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Guest images used

## Unattend Answer File

Used to automate the Windows 11 installation process. An answer file is an
XML file placed on a disk that the machine can mount. When the Windows
installer boots, it looks for this file and applies the
installation settings it contains.

Generated with <https://schneegans.de/windows/unattend-generator/> (commit 662d88d).

To reset the web form to the values we used, import autounattend.xml.
