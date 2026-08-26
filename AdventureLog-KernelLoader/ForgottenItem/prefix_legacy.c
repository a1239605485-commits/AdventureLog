/*******************************************************************************
 * File: prefix_legacy
 * Project: ForgottenItem
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

#include "mod_logger.h"
#include "tefkernel/patchlib/field.h"
#include "tefkernel/patchlib/method.h"
#include "tefkernel/patchlib/struct/array.h"
#include "tefkernel/patchlib/type.h"

// 字段句柄
static patch_handle_t g_SwordsHammersAxesPicks = PATCH_NULL;
static patch_handle_t g_GunsBows = PATCH_NULL;
static patch_handle_t g_MagicAndSummon = PATCH_NULL;

// Hook ID
static patch_hook_id_t g_init_hook_id = PATCH_HOOK_INVALID_ID;
static int g_initialized = 0;
static int g_patches_applied = 0;

// 应用所有补丁的函数
static void apply_patches(void) {
    // 获取 PrefixLegacy.ItemSets 类型的静态实例
    patch_handle_t setsType = patchlib_type_get_type("Terraria.GameContent.Prefixes", "PrefixLegacy.ItemSets");
    if (!setsType) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to get PrefixLegacy.ItemSets type");
        return;
    }

    // 获取静态字段的值（数组）
    if (g_SwordsHammersAxesPicks != PATCH_NULL) {
        patch_handle_t swordsArray = PATCH_NULL;
        patchlib_field_get_value(g_SwordsHammersAxesPicks, NULL, &swordsArray);
        if (swordsArray) {
            int val = 1;  // true
            patchlib_array_set(swordsArray, 4722, &val);  // 最初分形
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Set SwordsHammersAxesPicks[4722] = true");
        }
    }

    if (g_GunsBows != PATCH_NULL) {
        patch_handle_t gunsArray = PATCH_NULL;
        patchlib_field_get_value(g_GunsBows, NULL, &gunsArray);
        if (gunsArray) {
            int val = 1;  // true
            patchlib_array_set(gunsArray, 3850, &val);  // 埃特尼亚标枪
            patchlib_array_set(gunsArray, 3853, &val);  // 无趣弓
            patchlib_array_set(gunsArray, 4058, &val);  // 骷髅头弓
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Set GunsBows[3850,3853,4058] = true");
        }
    }

    if (g_MagicAndSummon != PATCH_NULL) {
        patch_handle_t magicArray = PATCH_NULL;
        patchlib_field_get_value(g_MagicAndSummon, NULL, &magicArray);
        if (magicArray) {
            int val = 1;  // true
            patchlib_array_set(magicArray, 2881, &val);  // 相位扭曲器
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Set MagicAndSummon[2881] = true");
        }
    }

    patchlib_free(setsType);
    g_patches_applied = 1;
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "前缀分类配置完成");
}

// Hook 函数 - 在 Main.Initialize 之后执行
static void On_Main_Initialize(patch_handle_t instance, void** args, void* result,
                                const patch_method_signature_t* sig) {
    (void)instance;
    (void)args;
    (void)result;
    (void)sig;

    // 确保只应用一次补丁
    if (!g_patches_applied) {
        apply_patches();
    }
}

// 初始化函数
void prefix_legacy_init(void) {
    if (g_initialized) return;

    // 获取 PrefixLegacy.ItemSets 类型
    patch_handle_t setsType = patchlib_type_get_type("Terraria.GameContent.Prefixes", "PrefixLegacy.ItemSets");
    if (!setsType) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to get PrefixLegacy.ItemSets type");
        return;
    }

    // 获取字段句柄（静态字段）
    g_SwordsHammersAxesPicks = patchlib_type_get_field(setsType, "SwordsHammersAxesPicks");
    if (!g_SwordsHammersAxesPicks) {
        mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "Failed to get SwordsHammersAxesPicks field");
    }

    g_GunsBows = patchlib_type_get_field(setsType, "GunsBows");
    if (!g_GunsBows) {
        mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "Failed to get GunsBows field");
    }

    g_MagicAndSummon = patchlib_type_get_field(setsType, "MagicAndSummon");
    if (!g_MagicAndSummon) {
        mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "Failed to get MagicAndSummon field");
    }

    patchlib_free(setsType);

    // 检查是否至少获取到部分字段
    if (g_SwordsHammersAxesPicks == PATCH_NULL &&
        g_GunsBows == PATCH_NULL &&
        g_MagicAndSummon == PATCH_NULL) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to get any prefix legacy fields");
        return;
    }

    // 获取 Main 类型并 Hook Initialize 方法
    patch_handle_t mainType = patchlib_type_get_type("Terraria", "Main");
    if (!mainType) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to get Main type");
        return;
    }

    // 获取 Initialize 方法（无参数）
    patch_handle_t initMethod = patchlib_type_get_method_by_param_count(mainType, "Initialize", 0);
    if (!initMethod) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to get Main.Initialize method");
        patchlib_free(mainType);
        return;
    }

    // 安装 Hook（post hook）
    g_init_hook_id = patchlib_install_prepost_hook(initMethod, NULL, On_Main_Initialize);

    if (g_init_hook_id == PATCH_HOOK_INVALID_ID) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to install Main.Initialize hook");
    } else {
        mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "PrefixLegacy Main.Initialize hook installed");
    }

    patchlib_free(initMethod);
    patchlib_free(mainType);

    g_initialized = 1;
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "PrefixLegacy module initialized");
}

// 清理函数
void prefix_legacy_cleanup(void) {
    if (g_init_hook_id != PATCH_HOOK_INVALID_ID) {
        patchlib_uninstall_hook(g_init_hook_id);
        g_init_hook_id = PATCH_HOOK_INVALID_ID;
    }

    if (g_SwordsHammersAxesPicks != PATCH_NULL) {
        patchlib_free(g_SwordsHammersAxesPicks);
        g_SwordsHammersAxesPicks = PATCH_NULL;
    }

    if (g_GunsBows != PATCH_NULL) {
        patchlib_free(g_GunsBows);
        g_GunsBows = PATCH_NULL;
    }

    if (g_MagicAndSummon != PATCH_NULL) {
        patchlib_free(g_MagicAndSummon);
        g_MagicAndSummon = PATCH_NULL;
    }

    g_initialized = 0;
    g_patches_applied = 0;

    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "PrefixLegacy module cleaned up");
}