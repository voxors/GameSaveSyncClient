<h3 align="center">
    <img src="https://raw.githubusercontent.com/voxors/GameSaveSyncClient/main/res/icon/GameSaveSyncClient.png" width="100" alt="GameSaveSyncClient Logo"/></br>
</h3>

<p align="center">
    <a href="https://github.com/voxors/GameSaveSyncClient/stargazers"><img src="https://img.shields.io/github/stars/voxors/GameSaveSyncClient?colorA=282c34&colorB=c678dd&style=for-the-badge"></a>
    <a href="https://github.com/voxors/GameSaveSyncClient/issues"><img src="https://img.shields.io/github/issues/voxors/GameSaveSyncClient?colorA=282c34&colorB=d19a66&style=for-the-badge"></a>
    <a href="https://github.com/voxors/GameSaveSyncClient/contributors"><img src="https://img.shields.io/github/contributors/voxors/GameSaveSyncClient?colorA=282c34&colorB=98c379&style=for-the-badge"></a>
</p>

# GameSaveSyncClient

GameSaveSyncClient is a cross‑platform Qt desktop application that synchronizes local game‑save folders with a remote GameSaveSync server.
It automatically detects game‑save paths on the local machine, uploads new or modified saves, and can download missing saves from the server.
The client communicates with the server via a simple JSON‑over‑HTTP API and stores the sync state in a local SQLite database.

## Build Requirements

- **Qt 6.x** (Widgets and Network modules). The project uses CMake to locate Qt; set the `Qt6_HOME` environment variable or let CMake find Qt automatically on the system path.
- **CMake ≥ 3.16** (used for project configuration).
- **Miniz** (automatically fetched by CMake – no separate install required).

## Running Requirements

- **A running GameSaveSync server** – the client needs the base URL, which is configurable in the Setup dialog the first time it starts.

## Build Instructions

```bash
# Create a build directory
mkdir build && cd build

cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build .
```

> In this mode, the build type is specified during configuration with `-DCMAKE_BUILD_TYPE`.
> No `--config` option is used.

## Running

```bash
# From the build directory
./bin/Release/GameSaveSyncClient
```

At first launch, the client will prompt you with the Setup dialog.

## License

GameSaveSyncClient is licensed under the [MIT license](LICENSE).
