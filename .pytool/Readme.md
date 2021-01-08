# Edk2-platforms Continuous Integration

## Basic Status

| Package                      | Windows VS2019 (IA32/X64)| Ubuntu GCC (IA32/X64/ARM/AARCH64) | Known Issues |
| :----                        | :-----                   | :----                             | :---         |
| Platfrom/ARM/JunoPkg         |                          | :heavy_check_mark:                | Spell checking in audit mode. CompilerCheck disabled (need a PlatformCI).

For more detailed status look at the test results of the latest CI run on the
repo readme.

## edk2 dependency

It is possible that the edk2-platforms repository relies on new modifications
in the edk2 repository. The edk2-platforms CI fetches the edk2 repository
via the edk2_ext_dep.yaml. The edk2 repository is hence treated as an external
dependency.
To use a custom edk2 repository:
- Place it inside the edk2-platforms folder
- Run the stuart_[update|build] commands with EDK2_REPO pointing to your
  custom edk2 repository.
E.g.:
stuart_update -c .pytool/CISettings.py EDK2_REPO=./my_edk2_repo`
stuart_ci_build -c .pytool/CISettings.py EDK2_REPO=./my_edk2_repo`

## Readme

As the content of the .pytool folder has been imported from the tianocore repository at:
https://github.com/tianocore/edk2
Please use the Readme.md that can be found there.
