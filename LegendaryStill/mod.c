/*******************************************************************************
 * File: mod
 * Project: LegendaryStill
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

// 字段句柄
static patch_handle_t g_numHits = PATCH_NULL;
static patch_handle_t g_lifeSteal = PATCH_NULL;

// Hook ID
static patch_hook_id_t g_ghostHeal_hook = PATCH_HOOK_INVALID_ID;
static patch_hook_id_t g_vampireHeal_hook = PATCH_HOOK_INVALID_ID;
static patch_hook_id_t g_resetEffects_hook = PATCH_HOOK_INVALID_ID;

// ghostHeal 前缀钩子 - 修改 numHits
// ReSharper disable once CppDFAConstantFunctionResult
static bool OnGhostHeal(patch_handle_t instance, void** args, const patch_method_signature_t* sig, void *result) {
    if (!instance || !args) return true;

    // 获取当前 numHits
    int hits = 0;
    patchlib_field_get_value(g_numHits, instance, &hits);

    // 设置新值：hits/2 或 1
    int newHits = hits / 2;
    if (newHits <= 0) newHits = 1;
    patchlib_field_set_value(g_numHits, instance, &newHits);

    mod_logger_write(MOD_LOG_LEVEL_DEBUG, "LegendaryStill",
                    "ghostHeal: numHits %d -> %d", hits, newHits);
    return true;
}

// vampireHeal 前缀钩子 - 修改伤害参数
// ReSharper disable once CppDFAConstantFunctionResult
static bool OnVampireHeal(patch_handle_t instance, void** args, const patch_method_signature_t* sig, void *result) {
    if (!args) return true;

    // args[1] 是 dmg 参数
    if (args[1]) {
        int* dmg = args[1];
        *dmg = *dmg * 3;  // 伤害 ×3，吸血比例提升至 22.5%
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "LegendaryStill",
                        "vampireHeal: dmg multiplied by 3 -> %d", *dmg);
    }

    return true;
}

// ResetEffects 后置钩子 - 设置 lifeSteal
static void OnResetEffects(patch_handle_t instance, void** args, void* result,
                           const patch_method_signature_t* sig) {
    (void)args;
    (void)result;
    (void)sig;
    if (!instance) return;

    static float value = 114514.0f;
    patchlib_field_set_value(g_lifeSteal, instance, &value);
    mod_logger_write(MOD_LOG_LEVEL_DEBUG, "LegendaryStill", "lifeSteal set to 114514");
}

static void init_mod(kernel_mod_handle_t* handle) {
    (void)handle;

    // 1. 获取 Projectile 类型和字段
    patch_handle_t projType = patchlib_type_get_type("Terraria", "Projectile");
    if (projType) {
        g_numHits = patchlib_type_get_field(projType, "numHits");

        // 安装 ghostHeal 钩子
        patch_handle_t ghostHeal = patchlib_type_get_method_by_param_count(projType, "ghostHeal", 3);
        if (ghostHeal) {
            g_ghostHeal_hook = patchlib_install_prepost_hook(ghostHeal, OnGhostHeal, NULL);
            patchlib_free(ghostHeal);
        }

        // 安装 vampireHeal 钩子
        patch_handle_t vampireHeal = patchlib_type_get_method_by_param_count(projType, "vampireHeal", 3);
        if (vampireHeal) {
            g_vampireHeal_hook = patchlib_install_prepost_hook(vampireHeal, OnVampireHeal, NULL);
            patchlib_free(vampireHeal);
        }

        patchlib_free(projType);
    }

    // 2. 获取 Player 类型和字段
    patch_handle_t playerType = patchlib_type_get_type("Terraria", "Player");
    if (playerType) {
        g_lifeSteal = patchlib_type_get_field(playerType, "lifeSteal");

        // 安装 ResetEffects 钩子
        patch_handle_t resetEffects = patchlib_type_get_method_by_param_count(playerType, "ResetEffects", 0);
        if (resetEffects) {
            g_resetEffects_hook = patchlib_install_prepost_hook(resetEffects, NULL, OnResetEffects);
            patchlib_free(resetEffects);
        }

        patchlib_free(playerType);
    }

    mod_logger_write(MOD_LOG_LEVEL_INFO, "LegendaryStill", "传奇依旧已加载");
}

static void cleanup_mod(kernel_mod_handle_t* handle) {
    (void)handle;
    if (g_ghostHeal_hook != PATCH_HOOK_INVALID_ID) patchlib_uninstall_hook(g_ghostHeal_hook);
    if (g_vampireHeal_hook != PATCH_HOOK_INVALID_ID) patchlib_uninstall_hook(g_vampireHeal_hook);
    if (g_resetEffects_hook != PATCH_HOOK_INVALID_ID) patchlib_uninstall_hook(g_resetEffects_hook);
    if (g_numHits) patchlib_free(g_numHits);
    if (g_lifeSteal) patchlib_free(g_lifeSteal);
    mod_logger_write(MOD_LOG_LEVEL_INFO, "LegendaryStill", "Unloaded");
}

static kernel_mod_info_t g_info = {
    .pkg_id = "eternal.future.legendarystill",
    .version_code = 20260606,
    .api_version = 1,
    .version = "1.0.0"
};

static kernel_mod_info_t* get_info(void) { return &g_info; }

static kernel_mod_ops_t g_ops = {
    .init_mod = init_mod,
    .cleanup_mod = cleanup_mod,
    .get_info = get_info
};

kernel_mod_ops_t* create_kernel_mod(void) { return &g_ops; }