/*******************************************************************************
 * File: mod
 * Project: Overclock
 * Created: 2026/6/6
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

#include "mod_core.h"
#include "mod_logger.h"
#include "tefkernel/patchlib/field.h"
#include "tefkernel/patchlib/method.h"

void (*mod_logger_write)(mod_log_level_t level, const char* tag, const char* fmt, ...) = NULL;

static patch_handle_t g_useTime = PATCH_NULL;
static patch_handle_t g_useAnimation = PATCH_NULL;
static patch_handle_t g_damage = PATCH_NULL;
static patch_handle_t g_createTile = PATCH_NULL;
static patch_handle_t g_createWall = PATCH_NULL;
static patch_hook_id_t g_hook_id = PATCH_HOOK_INVALID_ID;

static void OnSetDefaults(patch_handle_t item, void** args, void* result,
                          const patch_method_signature_t* sig) {
    (void)result;
    (void)sig;

    if (!item || !args) return;

    int type = *(int*)args[0];
    int damage = 0, tile = 0, wall = 0;

    patchlib_field_get_value(g_damage, item, &damage);
    patchlib_field_get_value(g_createTile, item, &tile);
    patchlib_field_get_value(g_createWall, item, &wall);

    if (damage > 0 || tile > 0 || wall > 0 || type == 1291 || type == 29 || type == 109) {
        int val1 = 1, val2 = 5;
        patchlib_field_set_value(g_useTime, item, &val1);
        patchlib_field_set_value(g_useAnimation, item, &val2);
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "Overclock", "加速物品 id=%d", type);
    }
}

static void init_mod(kernel_mod_handle_t* handle) {
    (void)handle;

    patch_handle_t itemType = patchlib_type_get_type("Terraria", "Item");
    if (!itemType) return;

    g_useTime = patchlib_type_get_field(itemType, "useTime");
    g_useAnimation = patchlib_type_get_field(itemType, "useAnimation");
    g_damage = patchlib_type_get_field(itemType, "damage");
    g_createTile = patchlib_type_get_field(itemType, "createTile");
    g_createWall = patchlib_type_get_field(itemType, "createWall");

    patch_handle_t method = patchlib_type_get_method_by_param_count(itemType, "SetDefaults", 2);
    if (method == PATCH_NULL) {
        method = patchlib_type_get_method_by_param_count(itemType, "SetDefaults", 3);
    }

    if (method) {
        g_hook_id = patchlib_install_prepost_hook(method, NULL, OnSetDefaults);
        patchlib_free(method);
    }

    patchlib_free(itemType);
    mod_logger_write(MOD_LOG_LEVEL_INFO, "Overclock", "Loaded");
}

static void cleanup_mod(kernel_mod_handle_t* handle) {
    (void)handle;
    if (g_hook_id != PATCH_HOOK_INVALID_ID) patchlib_uninstall_hook(g_hook_id);
    if (g_useTime) patchlib_free(g_useTime);
    if (g_useAnimation) patchlib_free(g_useAnimation);
    if (g_damage) patchlib_free(g_damage);
    if (g_createTile) patchlib_free(g_createTile);
    if (g_createWall) patchlib_free(g_createWall);
    mod_logger_write(MOD_LOG_LEVEL_INFO, "Overclock", "Unloaded");
}

static kernel_mod_info_t g_info = {
    .pkg_id = "eternal.future.overclock",
    .version_code = 20260606,
    .api_version = 1,
    .version = "1.1.1"
};

static kernel_mod_info_t* get_info(void) { return &g_info; }

static kernel_mod_ops_t g_ops = {
    .init_mod = init_mod,
    .cleanup_mod = cleanup_mod,
    .get_info = get_info
};

kernel_mod_ops_t* create_kernel_mod(void) { return &g_ops; }