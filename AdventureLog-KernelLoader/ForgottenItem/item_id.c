/*******************************************************************************
 * File: item_id
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
#include "tefkernel/patchlib/struct/array.h"
#include "tefkernel/patchlib/method.h"
#include "tefkernel/patchlib/type.h"

// 字段句柄（静态字段）
static patch_handle_t g_ShimmerTransformToItem = PATCH_NULL;
static patch_handle_t g_Deprecated = PATCH_NULL;
static patch_handle_t g_ItemsThatShouldNotBeInInventory = PATCH_NULL;

// Hook ID
static patch_hook_id_t g_init_hook_id = PATCH_HOOK_INVALID_ID;
static int g_initialized = 0;
static int g_patches_applied = 0;

// 应用所有补丁的函数
static void apply_patches(void) {
    // 修改 Deprecated 数组 - 全部设为 false
    if (g_Deprecated != PATCH_NULL) {
        patch_handle_t deprecatedArray = PATCH_NULL;
        patchlib_field_get_value(g_Deprecated, NULL, &deprecatedArray);
        if (deprecatedArray) {
            static bool val = false;
            patchlib_array_fill(deprecatedArray, &val);
            mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "已解除所有物品的废弃状态");
        }
    }

    // 修改 ItemsThatShouldNotBeInInventory 数组 - 全部设为 false
    if (g_ItemsThatShouldNotBeInInventory != PATCH_NULL) {
        patch_handle_t itemsArray = PATCH_NULL;
        patchlib_field_get_value(g_ItemsThatShouldNotBeInInventory, NULL, &itemsArray);
        if (itemsArray) {
            static bool val = false;
            patchlib_array_fill(itemsArray, &val);
            mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "已解除所有物品的禁装限制");
        }
    }

    // 设置 Shimmer 转换关系
    if (g_ShimmerTransformToItem != PATCH_NULL) {
        patch_handle_t shimmerArray = PATCH_NULL;
        patchlib_field_get_value(g_ShimmerTransformToItem, NULL, &shimmerArray);
        if (shimmerArray) {
            // 拜月 3357 <-> 3331
            int val1 = 3331, val2 = 3357;
            patchlib_array_set(shimmerArray, 3357, &val1);
            patchlib_array_set(shimmerArray, 3331, &val2);

            // 食人魔 3868 <-> 3861
            val1 = 3861; val2 = 3868;
            patchlib_array_set(shimmerArray, 3868, &val1);
            patchlib_array_set(shimmerArray, 3861, &val2);

            // 黑暗魔法师 3862 <-> 3867
            val1 = 3867; val2 = 3862;
            patchlib_array_set(shimmerArray, 3862, &val1);
            patchlib_array_set(shimmerArray, 3867, &val2);

            // 火星飞碟 2881 <-> 3358
            val1 = 3358; val2 = 2881;
            patchlib_array_set(shimmerArray, 2881, &val1);
            patchlib_array_set(shimmerArray, 3358, &val2);

            // 食人魔面具 3847 <-> 3865
            val1 = 3865; val2 = 3847;
            patchlib_array_set(shimmerArray, 3847, &val1);
            patchlib_array_set(shimmerArray, 3865, &val2);

            // 无趣弓 3854 <-> 3853
            val1 = 3853; val2 = 3854;
            patchlib_array_set(shimmerArray, 3854, &val1);
            patchlib_array_set(shimmerArray, 3853, &val2);

            // 邪教徒
            val1 = 2989; val2 = 2901;
            patchlib_array_set(shimmerArray, 2901, &val1);
            patchlib_array_set(shimmerArray, 2989, &val2);
            val1 = 2902; val2 = 2990;
            patchlib_array_set(shimmerArray, 2990, &val1);
            patchlib_array_set(shimmerArray, 2902, &val2);

            // 毒孢旗
            val1 = 1649; val2 = 3404;
            patchlib_array_set(shimmerArray, 3404, &val1);
            patchlib_array_set(shimmerArray, 1649, &val2);

            // 残手旗
            val1 = 1648; val2 = 3398;
            patchlib_array_set(shimmerArray, 3398, &val1);
            patchlib_array_set(shimmerArray, 1648, &val2);

            // 地牢玩具
            val1 = 1571; val2 = 1569;
            patchlib_array_set(shimmerArray, 1569, &val1);
            patchlib_array_set(shimmerArray, 1571, &val2);

            mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "已设置物品转换关系");
        }
    }

    g_patches_applied = 1;
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "物品配置修改完成");
}

// Hook 函数 - 在 Main.Initialize 之后执行
static void On_Main_Initialize(patch_handle_t instance, void** args, void* result,
                                const patch_method_signature_t* sig) {
    (void)args;
    (void)result;
    (void)sig;

    // 确保只应用一次补丁
    if (!g_patches_applied) {
        apply_patches();
    }
}

// 初始化函数 - 获取所有需要的字段句柄
void item_id_init(void) {
    if (g_initialized) return;

    // 获取 ItemID.Sets 类型
    patch_handle_t itemType = patchlib_type_get_type("Terraria.ID", "ItemID");
    if (!itemType) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to get ItemID type");
        return;
    }

    patch_handle_t setsType = patchlib_type_get_inner_type(itemType, "Sets");
    if (!setsType) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to get ItemID.Sets type");
        patchlib_free(itemType);
        return;
    }

    // 获取所有需要的字段句柄（静态字段）
    g_ShimmerTransformToItem = patchlib_type_get_field(setsType, "ShimmerTransformToItem");
    if (!g_ShimmerTransformToItem) {
        mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "Failed to get ShimmerTransformToItem field");
    }

    g_Deprecated = patchlib_type_get_field(setsType, "Deprecated");
    if (!g_Deprecated) {
        mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "Failed to get Deprecated field");
    }

    g_ItemsThatShouldNotBeInInventory = patchlib_type_get_field(setsType, "ItemsThatShouldNotBeInInventory");
    if (!g_ItemsThatShouldNotBeInInventory) {
        mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "Failed to get ItemsThatShouldNotBeInInventory field");
    }

    // 释放临时类型句柄
    patchlib_free(setsType);
    patchlib_free(itemType);

    // 检查是否至少获取到部分字段
    if (g_ShimmerTransformToItem == PATCH_NULL &&
        g_Deprecated == PATCH_NULL &&
        g_ItemsThatShouldNotBeInInventory == PATCH_NULL) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to get any required fields");
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
        mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "Main.Initialize hook installed");
    }

    patchlib_free(initMethod);
    patchlib_free(mainType);

    g_initialized = 1;
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "ItemID module initialized");
}

// 清理函数
void item_id_cleanup(void) {
    if (g_init_hook_id != PATCH_HOOK_INVALID_ID) {
        patchlib_uninstall_hook(g_init_hook_id);
        g_init_hook_id = PATCH_HOOK_INVALID_ID;
    }

    if (g_ShimmerTransformToItem != PATCH_NULL) {
        patchlib_free(g_ShimmerTransformToItem);
        g_ShimmerTransformToItem = PATCH_NULL;
    }

    if (g_Deprecated != PATCH_NULL) {
        patchlib_free(g_Deprecated);
        g_Deprecated = PATCH_NULL;
    }

    if (g_ItemsThatShouldNotBeInInventory != PATCH_NULL) {
        patchlib_free(g_ItemsThatShouldNotBeInInventory);
        g_ItemsThatShouldNotBeInInventory = PATCH_NULL;
    }

    g_initialized = 0;
    g_patches_applied = 0;

    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "ItemID module cleaned up");
}