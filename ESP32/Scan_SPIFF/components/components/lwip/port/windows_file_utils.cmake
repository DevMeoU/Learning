# Windows-specific file utilities for lwIP component
# This file provides helper functions to handle Windows file locking issues

# Function to safely delete files on Windows
# Uses CMD's DEL command with /F flag to force deletion even if file is in use
function(safe_delete_file file_path)
    if(CMAKE_HOST_WIN32)
        execute_process(
            COMMAND cmd.exe /C "DEL /F /Q \"${file_path}\""
            OUTPUT_QUIET
            ERROR_QUIET
        )
    else()
        file(REMOVE "${file_path}")
    endif()
endfunction()

# Function to safely copy files on Windows
# Uses CMD's COPY command which has better handling of file locks
function(safe_copy_file source_path target_path)
    if(CMAKE_HOST_WIN32)
        execute_process(
            COMMAND cmd.exe /C "COPY /Y \"${source_path}\" \"${target_path}\""
            OUTPUT_QUIET
            ERROR_QUIET
        )
    else()
        file(COPY "${source_path}" DESTINATION "${target_path}")
    endif()
endfunction()

# Function to add a delay before file operations
# This can help with file locking issues by giving the OS time to release locks
function(delay_before_file_op)
    if(CMAKE_HOST_WIN32)
        execute_process(
            COMMAND cmd.exe /C "ping -n 1 127.0.0.1 > nul"
            OUTPUT_QUIET
            ERROR_QUIET
        )
    endif()
endfunction()