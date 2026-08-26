/*******************************************************************************
 * File: item_drop_database
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

// 方法句柄
static patch_handle_t g_Common = PATCH_NULL;
static patch_handle_t g_RegisterToNPC = PATCH_NULL;

// Hook ID
static patch_hook_id_t g_populate_hook = PATCH_HOOK_INVALID_ID;
static int g_initialized = 0;

// 调用 Common 方法创建掉落规则
static patch_handle_t call_common(int dropID, int stackMin, int stackMax, int chanceDenom) {
    if (!g_Common) return PATCH_NULL;

    void* args[4] = { &dropID, &stackMin, &stackMax, &chanceDenom };
    patch_handle_t result = PATCH_NULL;
    patchlib_method_invoke_args(g_Common, NULL, &result, args);
    return result;
}

// 注册 NPC 掉落
static void register_to_npc(patch_handle_t database, int npcID, patch_handle_t rule) {
    if (!g_RegisterToNPC || !rule) return;

    void* args[2] = { &npcID, &rule };
    patch_handle_t result = PATCH_NULL;
    patchlib_method_invoke_args(g_RegisterToNPC, database, &result , args);
}

// Hook 函数 - 在 Populate 后添加掉落
static void OnPopulate(patch_handle_t instance, void** args, void* result,
                       const patch_method_signature_t* sig) {
    (void)args;
    (void)result;
    (void)sig;

    if (!instance) return;

    // 相位扭曲器 (2881) - 火星飞碟掉落
    patch_handle_t rule1 = call_common(2881, 50, 1, 1);
    if (rule1) {
        register_to_npc(instance, 395, rule1);  // 火星飞碟
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Added Phasic Warper (2881) to Martian Saucer");
    }

    // 无趣弓 (3853) - 黑暗魔法师掉落
    patch_handle_t rule2 = call_common(3853, 10, 1, 1);
    if (rule2) {
        register_to_npc(instance, 564, rule2);  // 黑暗魔法师
        register_to_npc(instance, 565, rule2);  // 黑暗魔法师
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Added Uninteresting Bow (3853) to Dark Mage");
    }

    // 埃特尼亚标枪 (3850) - 食人魔掉落
    patch_handle_t rule3 = call_common(3850, 10, 1, 1);
    if (rule3) {
        register_to_npc(instance, 561, rule3);  // 食人魔
        register_to_npc(instance, 562, rule3);  // 食人魔
        register_to_npc(instance, 563, rule3);  // 食人魔
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Added Eternia Javelin (3850) to Ogre");
    }

    // 哥布林面具 (3848) - 哥布林掉率
    patch_handle_t rule4 = call_common(3848, 10, 1, 1);
    if (rule4) {
        register_to_npc(instance, 552, rule4);
        register_to_npc(instance, 553, rule4);
        register_to_npc(instance, 554, rule4);
        register_to_npc(instance, 555, rule4);
        register_to_npc(instance, 556, rule4);
        register_to_npc(instance, 557, rule4);
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Added Goblin Mask (3848) to Goblins");
    }

    // 哥布林炸弹帽 (3849) - 哥布林掉率
    patch_handle_t rule5 = call_common(3849, 10, 1, 1);
    if (rule5) {
        register_to_npc(instance, 555, rule5);
        register_to_npc(instance, 556, rule5);
        register_to_npc(instance, 557, rule5);
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Added Goblin Bomb Cap (3849) to Goblins");
    }

    // 小妖魔雷管背包 (3851) - 小妖魔掉率
    patch_handle_t rule6 = call_common(3851, 10, 1, 1);
    if (rule6) {
        register_to_npc(instance, 572, rule6);
        register_to_npc(instance, 573, rule6);
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Added Kobold Dynamite Backpack (3851) to Kobolds");
    }

    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "NPC掉落配置完成");
}

// 初始化函数
void item_drop_database_init(void) {
    if (g_initialized) return;

    patch_handle_t ItemDropRule = patchlib_type_get_type("Terraria.GameContent.ItemDropRules", "ItemDropRule");
    patch_handle_t ItemDropDatabase = patchlib_type_get_type(
        "Terraria.GameContent.ItemDropRules", "ItemDropDatabase");

    // 获取方法
    g_Common = patchlib_type_get_method_by_param_count(ItemDropRule, "Common", 4);

    g_RegisterToNPC = patchlib_type_get_method_by_param_count(ItemDropDatabase, "RegisterToNPC", 2);

    // 获取 ItemDropDatabase 类型并安装 hook
    patch_handle_t dbType = patchlib_type_get_type(
        "Terraria.GameContent.ItemDropRules", "ItemDropDatabase");

    if (dbType) {
        patch_handle_t populateMethod = patchlib_type_get_method_by_param_count(
            dbType, "Populate", 0);

        if (populateMethod) {
            g_populate_hook = patchlib_install_prepost_hook(populateMethod, NULL, OnPopulate);
            patchlib_free(populateMethod);
        }

        patchlib_free(dbType);
    }

    patchlib_free(dbType);
    patchlib_free(ItemDropDatabase);
    patchlib_free(ItemDropRule);

    g_initialized = 1;
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "ItemDropDatabase initialized");
}

// 清理函数
void item_drop_database_cleanup(void) {
    if (g_populate_hook != PATCH_HOOK_INVALID_ID) {
        patchlib_uninstall_hook(g_populate_hook);
        g_populate_hook = PATCH_HOOK_INVALID_ID;
    }

    if (g_Common) {
        patchlib_free(g_Common);
        g_Common = PATCH_NULL;
    }

    if (g_RegisterToNPC) {
        patchlib_free(g_RegisterToNPC);
        g_RegisterToNPC = PATCH_NULL;
    }

    g_initialized = 0;
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "ItemDropDatabase cleaned up");
}