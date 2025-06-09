/*
 * Windows Dependency File Workaround
 * 
 * This file provides a workaround for Windows file locking issues during build.
 * It's specifically designed to address the problem where Ninja cannot delete
 * dependency files (.obj.d) because they're still being used by another process.
 * 
 * The file doesn't contain any functional code - it's only purpose is to be
 * compiled with special flags that help manage file locking on Windows.
 */

#include <stddef.h>

/* 
 * This dummy function is never called, but its compilation helps
 * ensure proper file handling on Windows platforms.
 */
void windows_dep_file_workaround(void) {
    /* This function intentionally does nothing */
}