# Edk2-platforms Continuous Integration

## Basic Status

| Package                      | Windows VS2019 (IA32/X64)| Ubuntu GCC (IA32/X64/ARM/AARCH64) | Known Issues |
| :----                        | :-----                   | :----                             | :---         |
| Platfrom/ARM/JunoPkg         |                          | :heavy_check_mark:                | Spell checking in audit mode. CompilerCheck disabled (need a PlatformCI).

For more detailed status look at the test results of the latest CI run on the
repo readme.

## edk2 submodule

It is possible that the edk2-platforms repository relies on new modifications
in the edk2 repository. The edk2-platforms CI uses the edk2 submodule. Thus,
the edk2 submodule might need to be updated to run the CI properly.

To rebase the edk2 submodule on the latest master, run:
* `git submodule update --remote --rebase edk2`

## Readme

As the content of the .pytool folder has been imported from the tianocore repository at:
https://github.com/tianocore/edk2
Please use the Readme.md that can be found there.


