# Local Windows build

This fork uses C++17/C++20 and Qt Quick (QML). The upstream repository is
`https://github.com/amnezia-vpn/amnezia-client` (`upstream` remote).

## Prerequisites

- Visual Studio 2022 Build Tools with the x64 C++ compiler and Windows SDK.
- CMake 3.25 or newer.
- Qt 6.10.1, MSVC 2022 x64, including `qtremoteobjects`, `qt5compat`, and `qtshadertools`.
- Python with `conan==2.28.0` installed in `.venv`.
- All Git submodules initialized.

## Build

Run from the repository root in PowerShell:

```powershell
# Compile Release and copy the application and dependencies into deploy/build/stage.
./deploy/build-local-windows.ps1
```

The default Qt directory is `$env:USERPROFILE\Qt\6.10.1\msvc2022_64`.
Use `-QtRoot` to override it, and `-Jobs` to control application build parallelism.

The script uses Conan Center and compiles missing dependencies locally, bypassing
the Amnezia prebuilt package server. The first build can take substantially longer
than subsequent builds. The Wintun recipe uses Brave's mirror of the official
archive and verifies the original upstream SHA-256 checksum.

`deploy/build/stage` is a staging directory, not an installer. Building and staging
do not register the Windows VPN service or install drivers. A VPN connection needs
the appropriate service and drivers. Launching this application may use the same
settings as an existing Amnezia installation.

The upstream release workflow supplies private endpoint configuration through
environment variables. Those values are not included in this fork, so this build
does not promise feature parity with official Amnezia Free/Premium releases.

Build outputs and `.venv` are ignored by Git. Conan caches dependencies under
`$env:USERPROFILE\.conan2`.
