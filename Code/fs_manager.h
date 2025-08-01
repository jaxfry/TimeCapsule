#ifndef FS_MANAGER_H
#define FS_MANAGER_H

#include <stddef.h>
#include <stdbool.h>

// Initialize the block device and file system.
bool fs_init(void);

// Mount both the public and private partitions.
bool fs_mount_partitions(void);

// Move a file from the public partition to the private one.
bool fs_move_to_private(const char* filename);

// Checks if a file is currently stored in the private area.
bool fs_is_file_in_private(void);

// Retrieves the unlock date from the private area.
bool fs_get_unlock_date(struct tm* unlock_date);

// Move a file from the private partition back to the public one.
bool fs_move_to_public(void);

// Scan the public partition for a file to be processed.
bool fs_find_file_in_public(char* found_filename, size_t max_len);

// Checks if a file's size has been stable for a short period.
bool fs_is_file_stable(const char* filename);

#endif // FS_MANAGER_H