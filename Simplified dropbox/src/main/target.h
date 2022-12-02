/**
 * @file target.h
 * @brief Module responding for creating and locking the folder
 * @author Dávid Šutor (xsutor@fi.muni.cz)
 * @date 2021-08-06
 */
#ifndef TARGET_H
#define TARGET_H

#include "settings.h"

#include <stdbool.h>

/**
 * @brief Try to create an empty TARGET directory.
 *
 * @param target_name  folder name string
 * @param settings     pointer to configuration structure
 *
 * @return  true if empty target directory exists or was successfully
 *          created or the non-empty directory exists and the force
 *          flag is set.
 *
 *          false if an error occurred or the non-empty-directory exists
 *          and the force flag wasn't set
 */
bool main_target_prepare_dir(const char *target_name,
                             const struct settings *settings);

/**
 * @brief Creates lock file and lock it.
 *
 * @param target_name  folder name string
 * @param settings     pointer to configuration structure
 *
 * @return  true on success;
 *          false otherwise
 */
bool main_target_lock_dir(const char *target_name, struct settings *settings);

/**
 * @brief Writes PID of the process to the lock file.
 *
 * @param settings     pointer to configuration structure
 *
 * @return  true on success;
 *          false otherwise
 */
bool main_target_write_pid_to_lock_file(const struct settings *settings);

#endif // TARGET_H
