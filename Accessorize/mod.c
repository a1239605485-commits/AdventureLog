/*******************************************************************************
 * File: mod
 * Project: Accessorize
 * Created: 2026/5/31
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

patch_handle_t g_field_headSlot = PATCH_NULL;
patch_handle_t g_field_bodySlot = PATCH_NULL;
patch_handle_t g_field_legSlot = PATCH_NULL;
patch_handle_t g_field_accessory = PATCH_NULL;

patch_hook_id_t g_set_defaults_hook_id = PATCH_HOOK_INVALID_ID;

void SetDefaults_postfix(patch_handle_t instance, void** args, void* result,
                        const patch_method_signature_t* sig_info) {
    if (!instance) return;

    // Check equipment slots
    int headSlot = 0, bodySlot = 0, legSlot = 0;
    int isAccessory = 0;

    // Get field values
    patchlib_field_get_value(g_field_headSlot, instance, &headSlot);
    patchlib_field_get_value(g_field_bodySlot, instance, &bodySlot);
    patchlib_field_get_value(g_field_legSlot, instance, &legSlot);
    patchlib_field_get_value(g_field_accessory, instance, &isAccessory);

    // Logic: if any equipment slot >= 0, mark as accessory
    if (headSlot >= 0 || bodySlot >= 0 || legSlot >= 0) {
        if (!isAccessory) {  // Avoid duplicate setting
            static int newAccessory = 1;
            patchlib_field_set_value(g_field_accessory, instance, &newAccessory);

            const int itemType = args[0] ? *(int*)args[0] : 0;
            mod_logger_write(MOD_LOG_LEVEL_INFO, "Accessorizec",
                            "Item[%d] marked as accessory (head=%d, body=%d, leg=%d)",
                            itemType, headSlot, bodySlot, legSlot);
        }
    }
}

static kernel_mod_info_t g_mod_info = {
    .pkg_id = "eternal.future.accessorizec",
    .version_code = 202605310,
    .api_version = 1,
    .version = "1.1.0"
};

static void init_mod(kernel_mod_handle_t* handle) {

    patch_handle_t itemType = patchlib_type_get_type("Terraria", "Item");
    g_field_headSlot = patchlib_type_get_field(itemType, "headSlot");
    g_field_bodySlot = patchlib_type_get_field(itemType, "bodySlot");
    g_field_legSlot = patchlib_type_get_field(itemType, "legSlot");
    g_field_accessory = patchlib_type_get_field(itemType, "accessory");

    patch_handle_t method_set_defaults = patchlib_type_get_method_by_param_count(itemType, "SetDefaults", 2);
    if (!method_set_defaults) {
        if (mod_logger_write) {
            mod_logger_write(MOD_LOG_LEVEL_WARNING, "Accessorizec",
                            "SetDefaults(2) not found, trying SetDefaults(3)");
        }
        method_set_defaults = patchlib_type_get_method_by_param_count(itemType, "SetDefaults", 3);
    }

    g_set_defaults_hook_id = patchlib_install_prepost_hook(method_set_defaults, NULL, SetDefaults_postfix);
    mod_logger_write(MOD_LOG_LEVEL_INFO, "Accessorizec",
                    "Hook installed on SetDefaults");

    patchlib_free(method_set_defaults);
}

static void cleanup_mod(kernel_mod_handle_t* handle) {
    patchlib_uninstall_hook(g_set_defaults_hook_id);
    patchlib_free(g_field_headSlot);
    patchlib_free(g_field_bodySlot);
    patchlib_free(g_field_legSlot);
    patchlib_free(g_field_accessory);
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
