[![build](https://github.com/alterware/aw-installer/workflows/Build/badge.svg)](https://github.com/alterware/aw-installer/actions)


# AlterWare: Installer
This is the tool we use to pull changes made from the release page of some of our clients and install it where we need to.

## Build
- Install [Premake5][premake5-link] and add it to your system PATH
- Clone this repository using [Git][git-link]
- Update the submodules using ``git submodule update --init --recursive``
- Run Premake with either of these two options ``premake5 vs2022`` (for Windows) or ``premake5 gmake2`` (for Linux/macOS)
  - On Windows, build the project via the solution file in ``build\aw-installer.sln``
  - On Linux/macOS, build the project using [Make][make-link] via the ``Makefile`` located in the ``build`` folder

**IMPORTANT**
Requirements for Unix systems:
- Compilation: Please use Clang
- Dependencies: Ensure the LLVM [C++ Standard library][libcxx-link] is installed
- Alternative compilers: If you opt for a different compiler such as GCC, use the [Mold][mold-link] linker
- Customization: Modifications to the Premake5.lua script may be required
- Platform support: Details regarding supported platforms are available in [build.yml](.github/workflows/build.yml)

[premake5-link]:          https://premake.github.io
[git-link]:               https://git-scm.com
[make-link]:              https://en.wikipedia.org/wiki/Make_(software)
[libcxx-link]:            https://libcxx.llvm.org/
[mold-link]:              https://github.com/rui314/mold
