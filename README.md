# Nx-Lyve-Cloud-Plugin

Nx-Lyve-Cloud-Plugin is a cloud storage plugin for Network Optix VMS's including Nx Witness. This plugin allows the user to easily configure their cloud and create a local mount of their cloud on their
system. This mount can then be used as a backup location for video recordings in the VMS. The plugins utilizes
[cloudfuse](https://github.com/Seagate/cloudfuse) to create and manage the local mount.

## Building the Nx-Lyve-Cloud-Plugin

The following are instructions to build our plugin from source.

The following are the currently supported platforms and architectures:

- Windows 10, 11 x64 (Microsoft Visual Studio).
- Linux Ubuntu 20.04, 22.04, 24.04 (GCC or Clang) x64

### Ubuntu

You should have a modern version of GCC or Clang and cmake installed on your system.

#### 1. Clone GitHub repository

```bash
git clone https://github.com/Seagate/nx-lyve-cloud-plugin
cd nx-lyve-cloud-plugin
```

#### 2. Run the build script

Run one of the following to build the plugin.

The following will default to your OS selected C compiler.

```bash
./build_plugin.sh --no-tests
```

The following will use gcc. You may also replace gcc/g++ with clang/clang++.

```bash
./build_plugin.sh --no-tests -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++
```

To be sure of compatibility, use a version 11 compiler:

```bash
sudo apt install g++-11 -y
./build_plugin.sh --no-tests -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++-11
```

After running the script the resulting .so file can be found at
`../nx-lyve-cloud-plugin-build/cloudfuse_plugin/libcloudfuse_plugin.so`.

### Windows

You should have a modern version of MSVC from Microsoft Visual Studio, cmake, and vcpkg installed on your system.

#### 1. Clone GitHub repository

```powershell
git clone https://github.com/Seagate/nx-lyve-cloud-plugin
cd nx-lyve-cloud-plugin
git checkout -b main origin/main
```

#### 2. Setup VCPKG with Cmake

Assuming you already have cmake installed and vcpkg, then we need to set the cmake toolchain to use the vcpkg toolchain.

```powershell
$Env:VCPKG_ROOT = "Path/To/Vcpkg"
```

#### 3. Run the build script

Run the following to build the plugin.

```powershell
./build_plugin.bat --no-tests --debug
```

After running the script the resulting `.so` file can be found at
`..\nx-lyve-cloud-plugin-build\cloudfuse_plugin\Debug\cloudfuse_plugin.dll`.

## Installing the Nx-Lyve-Cloud-Plugin

To install the plugin, copy the `.dll` or `.so` file to the plugins folder of the VMS.

Additionally, you need to install cloudfuse on your system. Follow install instructions for cloudfuse
[here](https://github.com/Seagate/cloudfuse).

After installing cloudfuse, and copying the plugin file, restart the VMS and the plugin will appear in the System
Administration settings windows. Then you can enter your S3 credentials and a mount will appear on the system connected
to the cloud. This can be selected as a backup mount and video files will be backed up to the cloud.

## License

Most of the project is licensed under the [Mozilla Public License](https://www.mozilla.org/en-US/MPL/2.0/) and the rest
is under MIT; check the specific license in each file to determine which is which.

### Third-Party Notices

See [notices](./NOTICE) for third party license notices.
