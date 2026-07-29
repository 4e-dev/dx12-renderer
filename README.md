# DX12 Game Engine
Author: Bao Bui

## Build Instructions

### Prerequisites

- C++ compiler (MSVC via Visual Studio recommended)
- Meson
- Ninja

Install dependencies:

```powershell
pip install meson ninja
```

### Configure Project

Generate the Visual Studio solution:

```powershell
meson setup build --backend=vs
```

### Build & Run

Open the generated solution in:

```text
build/
```

Then build/run through Visual Studio (`F5`).

### Reconfigure

If `meson.build` changes:

```powershell
meson setup build --reconfigure
```

If the build configuration becomes inconsistent:

```powershell
meson setup build --wipe --backend=vs
```
Otherwise run `rebuild.bat`, which automatically runs the prior command

### LSP for neovim

