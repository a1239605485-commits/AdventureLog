/*******************************************************************************
 * File: mod
 * Project: InstantRespawn
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
#include "tefkernel/patchlib/type.h"

void (*mod_logger_write)(mod_log_level_t level, const char* tag, const char* fmt, ...) = NULL;

// 字段句柄
static patch_handle_t g_respawnTimer = PATCH_NULL;
static patch_handle_t g_whoAmI = PATCH_NULL;

// Hook ID
static patch_hook_id_t g_hook_id = PATCH_HOOK_INVALID_ID;

// Hook 函数
static void OnUpdateDead(patch_handle_t instance, void** args, void* result,
                         const patch_method_signature_t* sig_info) {
    (void)args;
    (void)result;
    (void)sig_info;

    if (!instance) return;

    // 获取当前复活计时器
    int currentTimer = 0;
    patchlib_field_get_value(g_respawnTimer, instance, &currentTimer);

    // 如果大于 180，重置为 180
    if (currentTimer > 180) {
        int newTimer = 180;
        patchlib_field_set_value(g_respawnTimer, instance, &newTimer);

        int playerId = 0;
        patchlib_field_get_value(g_whoAmI, instance, &playerId);
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "InstantRespawn",
                        "Player[%d] timer reset: %d -> 180", playerId, currentTimer);
    }

    // 设置为 0，立即复活
    int zero = 0;
    patchlib_field_set_value(g_respawnTimer, instance, &zero);
}

// 模块初始化
static void init_mod(kernel_mod_handle_t* handle) {
    (void)handle;

    mod_logger_write(MOD_LOG_LEVEL_INFO, "InstantRespawn", "Loading...");

    // 获取 Player 类型
    patch_handle_t playerType = patchlib_type_get_type("Terraria", "Player");
    if (!playerType) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "InstantRespawn", "Can't get Player type");
        return;
    }

    // 获取字段
    g_respawnTimer = patchlib_type_get_field(playerType, "respawnTimer");
    g_whoAmI = patchlib_type_get_field(playerType, "whoAmI");

    if (!g_respawnTimer || !g_whoAmI) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "InstantRespawn", "Can't get fields");
        patchlib_free(playerType);
        return;
    }

    // 获取方法并安装 Hook
    patch_handle_t method = patchlib_type_get_method_by_param_count(playerType, "UpdateDead", 0);
    if (method) {
        g_hook_id = patchlib_install_prepost_hook(method, NULL, OnUpdateDead);
        if (g_hook_id != PATCH_HOOK_INVALID_ID) {
            mod_logger_write(MOD_LOG_LEVEL_INFO, "InstantRespawn", "Hook installed");
        }
        patchlib_free(method);
    }

    patchlib_free(playerType);
}

// 模块清理
static void cleanup_mod(kernel_mod_handle_t* handle) {
    (void)handle;

    if (g_hook_id != PATCH_HOOK_INVALID_ID) {
        patchlib_uninstall_hook(g_hook_id);
    }

    if (g_respawnTimer) patchlib_free(g_respawnTimer);
    if (g_whoAmI) patchlib_free(g_whoAmI);

    mod_logger_write(MOD_LOG_LEVEL_INFO, "InstantRespawn", "Unloaded");
}

// 模块信息
static kernel_mod_info_t g_info = {
    .pkg_id = "eternal.future.instantrespawn",
    .version_code = 20260606,
    .api_version = 1,
    .version = "1.0.0"
};

static kernel_mod_info_t* get_info(void) {
    return &g_info;
}

// 操作函数表
static kernel_mod_ops_t g_ops = {
    .init_mod = init_mod,
    .cleanup_mod = cleanup_mod,
    .get_info = get_info
};

// 入口点
kernel_mod_ops_t* create_kernel_mod(void) {
    return &g_ops;
}