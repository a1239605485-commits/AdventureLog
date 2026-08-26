/*******************************************************************************
 * File: InfiniteLegion
 * Project: Terraria-KernelLoader-Mods
 * Created: ${DATE}
 * Author: eternalfuture-e38299
 * Github: https://github.com/eternalfuture-e38299
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific terms and conditions governing
 * permissions and limitations under the License.
 *******************************************************************************/

#include <stddef.h>

#include "mod_core.h"
#include "mod_logger.h"

#include "tefkernel/patchlib/field.h"
#include "tefkernel/patchlib/method.h"

void (*mod_logger_write)(mod_log_level_t level, const char* tag, const char* fmt, ...) = NULL;

// Field handles
patch_handle_t g_field_maxTurrets = PATCH_NULL;
patch_handle_t g_field_maxTurretsOld = PATCH_NULL;
patch_handle_t g_field_maxMinions = PATCH_NULL;

// Hook ID
patch_hook_id_t g_reset_effects_hook_id = PATCH_HOOK_INVALID_ID;

/**
 * ResetEffects postfix hook
 * Executes after Player.ResetEffects() is called
 * Parameters:
 *   instance: Player instance
 *   args: argument array (ResetEffects has no parameters, so empty)
 *   result: return value (ResetEffects returns void, so NULL)
 *   sig_info: method signature info
 */
void ResetEffects_postfix(patch_handle_t instance, void** args, void* result,
                         const patch_method_signature_t* sig_info) {
    if (!instance) return;

    // Get current values
    int currentTurrets = 0;
    int currentMinions = 0;

    patchlib_field_get_value(g_field_maxTurrets, instance, &currentTurrets);
    patchlib_field_get_value(g_field_maxMinions, instance, &currentMinions);

    // Check if we need to set infinite
    if (currentTurrets != 999 || currentMinions != 999) {
        // Set infinite amounts
        static int INFINITE = 999;
        patchlib_field_set_value(g_field_maxTurrets, instance, &INFINITE);
        patchlib_field_set_value(g_field_maxTurretsOld, instance, &INFINITE);
        patchlib_field_set_value(g_field_maxMinions, instance, &INFINITE);

        mod_logger_write(MOD_LOG_LEVEL_INFO, "InfiniteLegion",
                        "[Player] Summon limit removed: Sentries %d->infinite, Minions %d->infinite",
                        currentTurrets, currentMinions);
    }
}

// ================== Module Lifecycle ==================

static kernel_mod_info_t g_mod_info = {
    .pkg_id = "eternal.future.infinitelegion",
    .version_code = 202605310,
    .api_version = 1,
    .version = "1.0.1"
};

/**
 * Module initialization
 * Corresponds to Load + Receive in old API
 */
static void init_mod(kernel_mod_handle_t* handle) {
    mod_logger_write(MOD_LOG_LEVEL_INFO, "InfiniteLegion", "Module initializing");

    // 1. Get Player type
    patch_handle_t playerType = patchlib_type_get_type("Terraria", "Player");
    if (!playerType) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "InfiniteLegion",
                        "Failed to get Player type");
        return;
    }

    mod_logger_write(MOD_LOG_LEVEL_DEBUG, "InfiniteLegion",
                    "Player type handle: %p", playerType);

    // 2. Get field handles
    g_field_maxTurrets = patchlib_type_get_field(playerType, "maxTurrets");
    g_field_maxTurretsOld = patchlib_type_get_field(playerType, "maxTurretsOld");
    g_field_maxMinions = patchlib_type_get_field(playerType, "maxMinions");

    mod_logger_write(MOD_LOG_LEVEL_DEBUG, "InfiniteLegion",
                    "Field handles: maxTurrets=%p, maxTurretsOld=%p, maxMinions=%p",
                    g_field_maxTurrets, g_field_maxTurretsOld, g_field_maxMinions);

    // 3. Get ResetEffects method and install hook
    // ResetEffects is a parameterless method, so parameter count is 0
    patch_handle_t method_reset_effects = patchlib_type_get_method_by_param_count(
        playerType, "ResetEffects", 0);

    if (!method_reset_effects) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "InfiniteLegion",
                        "Failed to get ResetEffects method");
        patchlib_free(playerType);
        return;
    }

    // 4. Install hook
    g_reset_effects_hook_id = patchlib_install_prepost_hook(
        method_reset_effects, NULL, ResetEffects_postfix);

    if (g_reset_effects_hook_id != PATCH_HOOK_INVALID_ID) {
        mod_logger_write(MOD_LOG_LEVEL_INFO, "InfiniteLegion",
                        "Hook installed on ResetEffects method");
    } else {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "InfiniteLegion",
                        "Failed to install hook");
    }

    // 5. Cleanup temporary handles
    patchlib_free(method_reset_effects);
    patchlib_free(playerType);

    mod_logger_write(MOD_LOG_LEVEL_INFO, "InfiniteLegion", "Module initialized successfully");
}

/**
 * Module cleanup
 * Corresponds to UnLoad in old API
 */
static void cleanup_mod(kernel_mod_handle_t* handle) {
    mod_logger_write(MOD_LOG_LEVEL_INFO, "InfiniteLegion", "Module cleanup");

    // Uninstall hook
    if (g_reset_effects_hook_id != PATCH_HOOK_INVALID_ID) {
        patchlib_uninstall_hook(g_reset_effects_hook_id);
        g_reset_effects_hook_id = PATCH_HOOK_INVALID_ID;
    }

    // Release field handles
    patchlib_free(g_field_maxTurrets);
    patchlib_free(g_field_maxTurretsOld);
    patchlib_free(g_field_maxMinions);

    g_field_maxTurrets = PATCH_NULL;
    g_field_maxTurretsOld = PATCH_NULL;
    g_field_maxMinions = PATCH_NULL;

    mod_logger_write(MOD_LOG_LEVEL_INFO, "InfiniteLegion", "Module cleanup completed");
}

static kernel_mod_info_t* get_info(void) {
    return &g_mod_info;
}

static kernel_mod_ops_t g_ops = {
    .init_mod = init_mod,
    .cleanup_mod = cleanup_mod,
    .get_info = get_info
};

kernel_mod_ops_t* create_kernel_mod(void) {
    return &g_ops;
}