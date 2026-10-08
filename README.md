# nccity-dsa-assignment-2

Checks that XML element tags are properly nested using functions, arrays, and
structures. `note.xml` reproduces the valid example in the supplied video;
`samples/invalid-missing.xml` reproduces its missing `</body>` example.

## Run in VS Code on this computer

1. Open this **folder** in VS Code: `D:\nccity dsa assignment-2`.
2. Open `main.c`. If VS Code asks about workspace trust, trust the folder after
   reviewing the files so its build tasks can run.
3. Press **Ctrl+Shift+B** to build, or **Ctrl+F5** to build and run immediately.
4. Accept `note.xml` at the file prompt to see `XML is valid`. Run again and enter
   `samples/invalid-missing.xml` to see `XML is invalid` and the error details.
5. Press **F5** to debug with breakpoints. Use **Terminal → Run Task → Test XML
   validator** to run the automated checks.

The setup uses Microsoft's installed C/C++ VS Code extension and a portable
GCC/GDB toolchain in `.tools/w64devkit`. No global PATH changes are needed.
The existing Visual Studio compiler on this computer lacks Windows SDK headers,
so the project uses GCC instead. On another Windows computer, install the C/C++
VS Code extension and run the included setup script once:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File setup.ps1
```

The script downloads the pinned official
[w64devkit 2.10.0 release](https://github.com/skeeto/w64devkit/releases/tag/v2.10.0),
checks its SHA256, and extracts it into the project. It requires internet access.
The compiler folder is excluded from GitHub; run setup again after cloning.

Use the configured run task or shortcuts above. The separate Code Runner
extension's play button has its own compiler configuration.

## PowerShell terminal commands

Run these from the project folder:

```powershell
.\build.cmd
.\build\main.exe
.\build\main.exe samples\invalid-missing.xml
.\build\main.exe samples\valid-features.xml
.\build\main.exe "C:\path with spaces\input.xml"
powershell -NoProfile -ExecutionPolicy Bypass -File tests\test.ps1
```

No argument means `note.xml` in the current directory. Exit codes are `0` for
valid structure, `1` for invalid/unsupported structure, and `2` for a usage,
file, or memory error. The build is C11 with warning checks and debug symbols.

The C file is also portable to GCC/Clang:

```sh
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o main
./main note.xml
```

## Submission

Submit **main.c** and **EXPLANATION.md**. You can upload the other project files
too so the examples and VS Code setup are available to the reviewer. Build
outputs, portable tools, and demo-review images are excluded by `.gitignore`. Read
[EXPLANATION.md](EXPLANATION.md) for the algorithm, XML research, and supported
scope.
