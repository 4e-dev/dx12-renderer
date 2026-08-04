:::::::::::::::::::::::::::::::::::::::::
:: Script for setting up `Ninja` build ::
:::::::::::::::::::::::::::::::::::::::::

@echo off
meson setup build-ninja --wipe --backend=ninja

:: Ensure `compile_commands.json` exists.
if exist "build-ninja\compile_commands.json" (
    echo Found compile_commands.json
) else (
    echo Missing compile_commands.json
)

:: Create a symbolic link for the language server.
IF EXIST "./compile_commands.json" (
    del \Q "./compile_commands.json"
)
copy "build-ninja\compile_commands.json" "compile_commands.json"
