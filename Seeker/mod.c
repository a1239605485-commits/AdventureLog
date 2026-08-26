/*******************************************************************************
 * File: mod
 * Project: Seeker
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

static patch_handle_t g_aiStyle = PATCH_NULL;
static patch_handle_t g_friendly = PATCH_NULL;
static patch_handle_t g_minion = PATCH_NULL;
static patch_handle_t g_sentry = PATCH_NULL;
static patch_hook_id_t g_hook_id = PATCH_HOOK_INVALID_ID;

static void OnSetDefaults(patch_handle_t proj, void** args, void* result,
                          const patch_method_signature_t* sig) {
    (void)result;
    (void)sig;

    if (!proj || !args) return;

    int type = *(int*)args[0];
    int aiStyle = 0;
    int friendly = 0;
    int minion = 0;
    int sentry = 0;

    patchlib_field_get_value(g_aiStyle, proj, &aiStyle);
    patchlib_field_get_value(g_friendly, proj, &friendly);
    patchlib_field_get_value(g_minion, proj, &minion);
    patchlib_field_get_value(g_sentry, proj, &sentry);

    // 检查是否需要修改
    if (friendly && aiStyle != 7 && aiStyle != 26 && !minion && !sentry &&
        type != 933 && aiStyle != 165 && type != 927) {
        int newStyle = 9;
        patchlib_field_set_value(g_aiStyle, proj, &newStyle);
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "Seeker",
                        "修正弹幕AI id=%d 原aiStyle=%d -> 9", type, aiStyle);
    }
}

static void init_mod(kernel_mod_handle_t* handle) {
    (void)handle;

    patch_handle_t projType = patchlib_type_get_type("Terraria", "Projectile");
    if (!projType) return;

    g_aiStyle = patchlib_type_get_field(projType, "aiStyle");
    g_friendly = patchlib_type_get_field(projType, "friendly");
    g_minion = patchlib_type_get_field(projType, "minion");
    g_sentry = patchlib_type_get_field(projType, "sentry");

    patch_handle_t method = patchlib_type_get_method_by_param_count(projType, "SetDefaults", 1);
    if (method) {
        g_hook_id = patchlib_install_prepost_hook(method, NULL, OnSetDefaults);
        patchlib_free(method);
    }

    patchlib_free(projType);
    mod_logger_write(MOD_LOG_LEVEL_INFO, "Seeker", "弹幕优化Mod已加载");
}

static void cleanup_mod(kernel_mod_handle_t* handle) {
    (void)handle;
    if (g_hook_id != PATCH_HOOK_INVALID_ID) patchlib_uninstall_hook(g_hook_id);
    if (g_aiStyle) patchlib_free(g_aiStyle);
    if (g_friendly) patchlib_free(g_friendly);
    if (g_minion) patchlib_free(g_minion);
    if (g_sentry) patchlib_free(g_sentry);
    mod_logger_write(MOD_LOG_LEVEL_INFO, "Seeker", "Unloaded");
}

static kernel_mod_info_t g_info = {
    .pkg_id = "eternal.future.seeker",
    .version_code = 20260606,
    .api_version = 1,
    .version = "1.2.0"
};

static kernel_mod_info_t* get_info(void) { return &g_info; }

static kernel_mod_ops_t g_ops = {
    .init_mod = init_mod,
    .cleanup_mod = cleanup_mod,
    .get_info = get_info
};

kernel_mod_ops_t* create_kernel_mod(void) { return &g_ops; }