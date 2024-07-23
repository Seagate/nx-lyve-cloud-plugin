# Nx-Lyve-Cloud-Plugin Changelog #

## **0.1.2** ##

July 23 2024
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Bug Fixes ###

- [#35](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/35) Fix regex for secret and access keys
- [#34](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/34) Allow retrying mount if settings are the same but cloudfuse is not mounted
- [#30](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/30) Select driver letter to mount based on available letters, not just Z:

## **0.1.1** ##

July 19 2024
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Bug Fixes ###

- [#31](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/31) Fix Windows builds to correctly run in release mode

## **0.1.0** ##

July 11 2024
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Features ###

- Enables VMS to easily connect to S3 based clouds (including Seagate Lyve Cloud) by running cloudfuse in the background
- Runs on Network Optix based VMS's for both Windows and Ubuntu Linux
