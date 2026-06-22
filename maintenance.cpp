// ===========================================================================
//  Windows System Maintenance Utility  --  version 1.0
// ---------------------------------------------------------------------------
//  Purpose : Run a fixed sequence of safe Windows maintenance tasks (disk
//            cleanup, system file check, update / DNS / prefetch / Chrome
//            cache clears, DISM component-store cleanup, and winget
//            application updates), stopping at the first task that fails. The
//            program self-elevates to administrator, which these tasks
//            require.
//
//  Platform: Windows only -- uses the Win32 API and system() shell commands.
//
//  Build (MinGW / GCC):
//      g++ -static -static-libgcc -static-libstdc++ -O2 -s \
//          -o maintenance.exe maintenance.cpp
//  Build (Visual Studio, Release):
//      cl /MT /O2 maintenance.cpp /Fe:maintenance.exe
//
//  Style  : Follows the NASA Glenn NPARC Alliance programming guidelines
//           (maintainability > portability > efficiency), adapted from
//           Fortran 90 to C++ -- lines under 80 columns, one statement per
//           line, a purpose header on every function, consistent "//-----"
//           comment leaders, named constants for magic numbers, and flat,
//           modular control flow (no deeply nested conditionals).
// ===========================================================================

#include <windows.h>
#include <shellapi.h>
#include <cstdlib>
#include <iostream>
#include <string>

using std::cout;
using std::cin;
using std::string;

//-----Named constants -------------------------------------------------------
const int  SEPARATOR_WIDTH = 50;    // Columns in the "=" banner separators
const char SEPARATOR_CHAR  = '=';   // Character the separators are drawn with


// ---------------------------------------------------------------------------
//  Purpose : Print one horizontal separator line of the standard width.
// ---------------------------------------------------------------------------
void printSeparator() {
    cout << string(SEPARATOR_WIDTH, SEPARATOR_CHAR) << "\n";
}


// ---------------------------------------------------------------------------
//  Purpose : Report whether the current process is running with administrator
//            privileges. Returns true when it is.
// ---------------------------------------------------------------------------
bool isRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    //-----Build the well-known SID for the local Administrators group
    if (AllocateAndInitializeSid(&ntAuthority, 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }

    return isAdmin == TRUE;
}


// ---------------------------------------------------------------------------
//  Purpose : Relaunch this executable with administrator privileges via the
//            shell "runas" verb. Returns true when the elevated instance was
//            started (the caller should then exit the current instance).
// ---------------------------------------------------------------------------
bool restartAsAdmin() {
    char szPath[MAX_PATH];

    if (!GetModuleFileNameA(NULL, szPath, ARRAYSIZE(szPath))) {
        return false;
    }

    //-----Ask the shell to launch this same file, elevated
    SHELLEXECUTEINFOA sei = { sizeof(sei) };
    sei.lpVerb = "runas";              // Request elevation
    sei.lpFile = szPath;               // Path to the current executable
    sei.hwnd   = NULL;
    sei.nShow  = SW_NORMAL;

    if (ShellExecuteExA(&sei)) {
        return true;                   // Elevated instance launched
    }

    //-----Report why elevation did not happen
    DWORD error = GetLastError();
    if (error == ERROR_CANCELLED) {
        cout << "User cancelled the elevation request.\n";
    } else {
        cout << "Failed to restart as administrator. Error: "
             << error << "\n";
    }
    return false;
}


// ---------------------------------------------------------------------------
//  Purpose : Run Windows Disk Cleanup using its pre-saved settings profile.
//            Returns 0 on success, -1 on failure.
// ---------------------------------------------------------------------------
int runCleanMgr() {
    cout << "Starting automated Disk Cleanup (cleanmgr /sagerun:1)...\n";
    cout << "Running with predefined cleanup settings.\n\n";

    int status = system("cleanmgr /sagerun:1");

    if (status == 0) {
        cout << "Automated Disk Cleanup completed successfully.\n";
        return 0;
    }
    cout << "Failed to run automated Disk Cleanup. Error code: "
         << status << "\n";
    cout << "Note: run 'cleanmgr /sageset:1' first to configure "
         << "settings.\n";
    return -1;
}


// ---------------------------------------------------------------------------
//  Purpose : Run the System File Checker (sfc /scannow) to repair protected
//            system files. Returns 0 on success, -1 on failure.
// ---------------------------------------------------------------------------
int runSFC() {
    cout << "Starting System File Checker (sfc /scannow)...\n";
    cout << "This may take several minutes to complete.\n";
    cout << "Please wait...\n\n";

    int status = system("sfc /scannow");

    if (status == 0) {
        cout << "System File Checker completed successfully.\n";
        return 0;
    }
    cout << "System File Checker failed. Error code: " << status << "\n";
    return -1;
}


// ---------------------------------------------------------------------------
//  Purpose : Delete the Windows Update download cache to free disk space.
//            Returns 0 on success, -1 on failure.
// ---------------------------------------------------------------------------
int clearUpdateCache() {
    cout << "Clearing Windows Update download cache...\n";
    cout << "This will delete temporary update files to free space.\n\n";

    int status = system(
        "del /q /f /s C:\\Windows\\SoftwareDistribution\\Download\\*");

    if (status == 0) {
        cout << "Windows Update cache cleared successfully.\n";
        return 0;
    }
    cout << "Failed to clear Windows Update cache. Error code: "
         << status << "\n";
    return -1;
}


// ---------------------------------------------------------------------------
//  Purpose : Flush the DNS resolver cache so name lookups start fresh.
//            Returns 0 on success, -1 on failure.
// ---------------------------------------------------------------------------
int flushDNS() {
    cout << "Flushing DNS cache...\n";
    cout << "This will clear DNS resolver cache for fresh lookups.\n\n";

    int status = system("ipconfig /flushdns");

    if (status == 0) {
        cout << "DNS cache flushed successfully.\n";
        return 0;
    }
    cout << "Failed to flush DNS cache. Error code: " << status << "\n";
    return -1;
}


// ---------------------------------------------------------------------------
//  Purpose : Delete the Windows Prefetch cache (application launch data).
//            Returns 0 on success, -1 on failure.
// ---------------------------------------------------------------------------
int clearPrefetch() {
    cout << "Clearing Windows Prefetch cache...\n";
    cout << "This will delete application prefetch data.\n\n";

    int status = system("del /q /f /s C:\\Windows\\Prefetch\\*");

    if (status == 0) {
        cout << "Prefetch cache cleared successfully.\n";
        return 0;
    }
    cout << "Failed to clear Prefetch cache. Error code: " << status << "\n";
    return -1;
}


// ---------------------------------------------------------------------------
//  Purpose : Run DISM component-store cleanup to remove superseded Windows
//            components. Returns 0 on success, -1 on failure.
// ---------------------------------------------------------------------------
int dismCleanup() {
    cout << "Starting DISM Component Store cleanup...\n";
    cout << "This cleans up superseded Windows components and may "
         << "take several minutes.\n\n";

    int status = system(
        "dism /Online /Cleanup-Image /StartComponentCleanup");

    if (status == 0) {
        cout << "DISM Component Store cleanup completed successfully.\n";
        return 0;
    }
    cout << "DISM Component Store cleanup failed. Error code: "
         << status << "\n";
    return -1;
}


// ---------------------------------------------------------------------------
//  Purpose : Delete Google Chrome's cached internet files for the default
//            profile. Returns 0 on success, -1 on failure (which also occurs
//            when Chrome is not installed or the cache is already empty).
// ---------------------------------------------------------------------------
int clearChromeCache() {
    cout << "Clearing Google Chrome cache...\n";
    cout << "This will delete Chrome's temporary cached data.\n\n";

    int status = system(
        "del /q /s \"%LocalAppData%\\Google\\Chrome\\User Data"
        "\\Default\\Cache\\*\"");

    if (status == 0) {
        cout << "Chrome cache cleared successfully.\n";
        return 0;
    }
    cout << "Failed to clear Chrome cache (Chrome may not be installed "
         << "or the cache is already empty).\n";
    return -1;
}


// ---------------------------------------------------------------------------
//  Purpose : Update every winget-managed application. Returns 0 on success,
//            -1 on failure. Runs last in the maintenance sequence.
// ---------------------------------------------------------------------------
int wingetUpgradeAll() {
    cout << "Updating all applications via winget...\n";
    cout << "This may take several minutes depending on available "
         << "updates.\n\n";

    int status = system("winget upgrade --all");

    if (status == 0) {
        cout << "Winget application updates completed successfully.\n";
        return 0;
    }
    cout << "Winget upgrade failed or no updates available. Error code: "
         << status << "\n";
    cout << "Note: winget requires Windows 10 version 1709 or later.\n";
    return -1;
}


//-----Maintenance-task table ------------------------------------------------
//  Each entry pairs a display name with the function that performs the task.
//  main() runs them in this order and stops at the first failure. Adding,
//  removing, or reordering a task is a one-line change here -- which is why
//  this table replaces the original deeply nested if/else chain.
typedef int (*MaintenanceTask)();

struct TaskEntry {
    const char*     name;          // Name shown in the progress banner
    MaintenanceTask run;           // Function that performs the task
};

const TaskEntry TASKS[] = {
    { "Disk Cleanup",            runCleanMgr      },
    { "System File Checker",     runSFC           },
    { "Windows Update Cache",    clearUpdateCache },
    { "DNS Cache Flush",         flushDNS         },
    { "Prefetch Cache",          clearPrefetch    },
    { "DISM Component Store",    dismCleanup      },
    { "Chrome Cache",            clearChromeCache },
    { "Application Updates",     wingetUpgradeAll }
};

const int TASK_COUNT = (int)(sizeof(TASKS) / sizeof(TASKS[0]));


// ---------------------------------------------------------------------------
//  Purpose : Program entry point. Ensure administrator rights (self-elevating
//            if needed), then run the maintenance tasks in order, stopping at
//            the first failure. Returns 0 when every task succeeded.
// ---------------------------------------------------------------------------
int main() {
    cout << "=== Windows System Maintenance Utility v1.0 ===\n";
    cout << "Safe system utility for disk cleanup and file checking\n";
    printSeparator();
    cout << "\n";

    //-----Make sure we are elevated; relaunch as admin if we are not
    if (!isRunningAsAdmin()) {
        cout << "This program requires administrator privileges.\n";
        cout << "Attempting to restart with administrator rights...\n\n";

        if (restartAsAdmin()) {
            cout << "Restarting with administrator privileges...\n";
            return 0;                  // Elevated instance will continue
        }
        cout << "Failed to obtain administrator privileges.\n";
        cout << "Please manually run this program as administrator.\n\n";
        cout << "Press Enter to exit...";
        cin.get();
        return 1;
    }

    cout << "Running with administrator privileges - Good!\n\n";
    cout << "Starting automated maintenance tasks...\n\n";

    //-----Run each task in turn, stopping at the first failure
    int result = 0;
    for (int i = 0; i < TASK_COUNT; ++i) {
        printSeparator();
        cout << "Task " << (i + 1) << " of " << TASK_COUNT << ": "
             << TASKS[i].name << "\n";
        printSeparator();
        cout << "\n";

        result = TASKS[i].run();
        if (result != 0) {
            cout << "\n" << TASKS[i].name
                 << " failed. Skipping remaining tasks.\n";
            break;
        }
        cout << "\n";
    }

    //-----Final summary
    cout << "=== Maintenance Complete ===\n";
    if (result == 0) {
        cout << "All selected tasks completed successfully.\n";
    } else {
        cout << "Some tasks may have encountered issues. "
             << "Check the output above.\n";
    }

    cout << "\nPress Enter to exit...";
    cin.ignore();
    cin.get();

    return result;
}
