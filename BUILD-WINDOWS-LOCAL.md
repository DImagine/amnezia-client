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

## Quick split-tunneling controls

For the feature's goals, UX decisions, failure handling, and implementation map,
see [QUICK-SPLIT-TUNNELING.md](QUICK-SPLIT-TUNNELING.md) (Russian).

The Windows home page offers three compact presets: All traffic, Addresses, and Apps.
The original split-tunneling settings entry remains clickable above the presets.
The selected preset has a neutral outline while disconnected, a golden glow while
connected, and a pulsing target outline during automatic switching. Labels use the
same 14 px size as the original split-tunneling entry.
They preserve the configured lists and their inclusion/exclusion rules. Selecting
a different preset while connected waits for disconnection, applies the preset,
then uses the regular connection/configuration-validation flow. While disconnected,
selecting a preset only saves the settings. Server-controlled routing disables the
presets. Existing mixed settings are displayed explicitly until a preset is chosen.

Automatic preset changes suppress intermediate Windows VPN-state notifications.
After success, the stock connected notification is sent once after 1.5 seconds
without another preset change. A new change resets that wait; a failure or ordinary
disconnect cancels it. Ordinary connection notifications and tray state updates
retain their existing behavior.

Quick reconnection validation is tagged with a request ID and the selected server/
protocol. Timeout, cancellation, or a selection change invalidates late results
(including API captcha replies), and stops an in-flight reconnect. Specific stock
errors are shown once; the quick-switch fallback message is used when no specific
error is available. Settings already applied remain selected if reconnection fails.
The disconnect/reconnect deadlines are 30/120 seconds respectively.

The current build is staged in `deploy/build/stage-quick-split-glow`. Exit the
previous GUI from its tray menu before launching its `AmneziaVPN.exe`. The existing
Windows service and saved settings are reused; no reinstall is performed.

The first build of this feature is staged separately in `deploy/build/stage-quick-split`
so an already running baseline build in `stage` can remain open.

The isolated tests below do not use the VPN service or the user's saved servers:

```powershell
# Build the coordinator and actual QML component tests independently of the client.
$qtRoot = "$env:USERPROFILE/Qt/6.10.1/msvc2022_64"
cmake -S client/tests/quickSplit -B deploy/build/quick-split-tests "-DCMAKE_PREFIX_PATH=$qtRoot"
cmake --build deploy/build/quick-split-tests --config Release
$env:PATH = "$qtRoot/bin;$env:PATH"
$env:QT_PLUGIN_PATH = "$qtRoot/plugins"
# Compile Russian strings for the narrow-window layout checks.
& "$qtRoot/bin/lrelease.exe" client/translations/amneziavpn_ru_RU.ts -qm deploy/build/quick-split-tests/ru.qm
$env:QUICK_SPLIT_TRANSLATION = "$PWD/deploy/build/quick-split-tests/ru.qm"
ctest --test-dir deploy/build/quick-split-tests -C Release --output-on-failure
```
