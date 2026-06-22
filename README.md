[README.md](https://github.com/user-attachments/files/29187569/README.md)
# Windows System Maintenance Utility

A small, self-contained Windows command-line tool that runs a fixed sequence of
safe system-maintenance tasks in one pass. It self-elevates to administrator
(which the tasks require) and stops at the first task that fails so problems are
easy to spot.

## What it does

The tasks run in this order, each reporting success or failure:

| # | Task | Command behind it |
|---|------|-------------------|
| 1 | Disk Cleanup | `cleanmgr /sagerun:1` |
| 2 | System File Checker | `sfc /scannow` |
| 3 | Windows Update Cache clear | deletes `C:\Windows\SoftwareDistribution\Download\*` |
| 4 | DNS Cache flush | `ipconfig /flushdns` |
| 5 | Prefetch Cache clear | deletes `C:\Windows\Prefetch\*` |
| 6 | DISM Component Store cleanup | `dism /Online /Cleanup-Image /StartComponentCleanup` |
| 7 | Chrome Cache clear | deletes the default Chrome profile cache |
| 8 | Application Updates | `winget upgrade --all` |

If any task fails, the remaining tasks are skipped and the program reports which
one stopped the run.

## Requirements

- **Windows 10 (1709 or later) or Windows 11** — task 8 (`winget`) needs this;
  the earlier tasks work on older Windows too.
- **Administrator privileges** — the program detects when it is not elevated and
  relaunches itself with a UAC prompt.
- A C++ compiler to build it: **MinGW/GCC** (`g++`) or **Visual Studio** (`cl`).

## Build

From the project folder:

```bat
:: MinGW / GCC (via the included Makefile)
make

:: ...or directly
g++ -std=c++11 -O2 -static -static-libgcc -static-libstdc++ -s ^
    -o maintenance.exe maintenance.cpp

:: Visual Studio (Developer Command Prompt)
cl /MT /O2 maintenance.cpp /Fe:maintenance.exe
```

`make clean` removes the built executable.

## Usage

Run `maintenance.exe` (double-click or from a terminal). If it is not already
elevated it will request administrator rights, then run all tasks automatically
and print a summary. No arguments or interactive menu — it is intentionally a
one-click, run-everything tool.

## Safety notes

- Every task is a standard, reversible Windows maintenance operation (cache
  clears, file checks, component cleanup). Nothing here deletes personal files.
- **Task 8 auto-updates every `winget`-managed application.** If you do not want
  unattended app updates, remove the `"Application Updates"` row from the
  `TASKS[]` table in `maintenance.cpp` and rebuild.
- The Chrome cache clear targets the **Default** profile only; it reports a
  harmless failure if Chrome is not installed.

## Project layout

```
windows-maintenance-utility/
├── maintenance.cpp       Source code
├── maintenance.cpp.bak   Original pre-cleanup source (kept for reference)
├── Makefile              Build / clean targets (MinGW)
└── README.md             This file
```

## Code style

The source follows the **NASA Glenn NPARC Alliance programming guidelines**
(maintainability > portability > efficiency), adapted from their Fortran 90
style guide to C++: lines under 80 columns, one statement per line, a purpose
header on every function, consistent `//-----` comment leaders, named constants
for magic numbers, specific `using` declarations, and flat, table-driven control
flow instead of deeply nested conditionals. Adding, removing, or reordering a
task is a one-line edit to the `TASKS[]` table.

## Customizing the task list

Open `maintenance.cpp` and edit the `TASKS[]` table near `main()`:

```cpp
const TaskEntry TASKS[] = {
    { "Disk Cleanup",         runCleanMgr      },
    { "System File Checker",  runSFC           },
    // ... add, remove, or reorder rows here ...
    { "Application Updates",  wingetUpgradeAll }
};
```

Each row pairs a display name with the function that performs the task. To add a
new task, write a function returning `0` on success / `-1` on failure and add a
row. The progress banner and the count (`Task N of M`) update automatically.

## License / disclaimer

Provided as-is, for personal system maintenance. Review the commands before
running on systems you do not control.
