# Nx-Lyve-Cloud-Plugin Changelog #

## **0.5.1** ##

March 6th 2025
This release is packaged with Cloudfuse [v1.9.1](https://github.com/Seagate/cloudfuse/releases/tag/v1.9.1).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Bug Fixes ###

- [#79](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/79) Fix issue with path to installed version of cloudfuse on some Linux systems
- [#78](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/78) Fix issue when running Windows installer where path has spaces

## **0.5.0** ##

February 4th 2025
This release is packaged with Cloudfuse [v1.8.0](https://github.com/Seagate/cloudfuse/releases/tag/v1.8.0).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Changes ###

- [#74](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/74) Add ability for cloud storage to correctly present the servers stored capacity in the cloud

## **0.4.2** ##

January 16th 2025
This release is packaged with Cloudfuse [v1.7.4](https://github.com/Seagate/cloudfuse/releases/tag/v1.7.4).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

## **0.4.1** ##

January 13th 2025
This release is packaged with Cloudfuse [v1.7.3](https://github.com/Seagate/cloudfuse/releases/tag/v1.7.3).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Bug Fixes ###

- [#69](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/69) Remove field validation, which was causing false error messages

## **0.4.0** ##

December 12th 2024
This release is packaged with Cloudfuse [v1.7.2](https://github.com/Seagate/cloudfuse/releases/tag/v1.7.2).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Changes ###

- [#65](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/65) Update nx library version

## **0.3.2** ##

November 18th 2024
This release is packaged with Cloudfuse [v1.7.1](https://github.com/Seagate/cloudfuse/releases/tag/v1.7.1).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Bug Fixes ###

- [#63](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/63) Improve template config error handling

## **0.3.1** ##

November 8th 2024
This release is packaged with Cloudfuse [v1.7.1](https://github.com/Seagate/cloudfuse/releases/tag/v1.7.1).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Bug Fixes ###

- [#61](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/61) Fix libc version error with builds for Ubuntu 24.04

## **0.3.0** ##

November 6th 2024
This release is packaged with Cloudfuse [v1.7.0](https://github.com/Seagate/cloudfuse/releases/tag/v1.7.0).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

## **0.2.2** ##

October 22 2024
This release is packaged with Cloudfuse [v1.6.0](https://github.com/Seagate/cloudfuse/releases/tag/v1.6.0).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Changes ###

- [#57](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/57) Add advanced option to set bucket capacity

## **0.2.1** ##

August 29 2024
This release is packaged with Cloudfuse [v1.5.0](https://github.com/Seagate/cloudfuse/releases/tag/v1.5.0).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Changes ###

- [#47](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/47) Allow config template to be modified by user
- [#52](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/52) Provide status feedback directly in settings UI
- [#45](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/45) Write cloudfuse log output to a file (~/.cloudfuse/cloudfuse.log) instead of syslog
- [#50](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/50) Write plugin log output to a file (nx_ini/mediaserver_stderr.log)

### Bug Fixes ###

- [#48](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/48) Improve plugin log output
- [#46](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/46) Fix Windows install script privileges
- [#51](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/51) Don't lose users settings on cloud connection failure

## **0.2.0** ##

August 7 2024
This release is packaged with Cloudfuse [v1.4.0](https://github.com/Seagate/cloudfuse/releases/tag/v1.4.0).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Bug Fixes ###

- [#41](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/41) Fixed issue with multiple servers mounting on same bucket
- [#37](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/37) Create simple install scripts for Linux and Windows
- [#38](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/38) Release plugin for arm

## **0.1.2** ##

July 23 2024
This release was tested with Cloudfuse [v1.3.2](https://github.com/Seagate/cloudfuse/releases/tag/v1.3.2).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Bug Fixes ###

- [#35](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/35) Fix regex for secret and access keys
- [#34](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/34) Allow retrying mount if settings are the same but cloudfuse is not mounted
- [#30](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/30) Select driver letter to mount based on available letters, not just Z:

## **0.1.1** ##

July 19 2024
This release was tested with Cloudfuse [v1.3.1](https://github.com/Seagate/cloudfuse/releases/tag/v1.3.1).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Bug Fixes ###

- [#31](https://github.com/Seagate/nx-lyve-cloud-plugin/pull/31) Fix Windows builds to correctly run in release mode

## **0.1.0** ##

July 11 2024
This release was tested with Cloudfuse [v1.3.1](https://github.com/Seagate/cloudfuse/releases/tag/v1.3.1).
This release includes all features planned for the 1.0.0 release of the nx-lyve-cloud-plugin, which will be released after additional bug fixes and testing.

### Features ###

- Enables VMS to easily connect to S3 based clouds (including Seagate Lyve Cloud) by running cloudfuse in the background
- Runs on Network Optix based VMS's for both Windows and Ubuntu Linux
