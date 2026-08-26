/*******************************************************************************
 * File: item
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

// 字段句柄
static patch_handle_t g_useStyle = PATCH_NULL;
static patch_handle_t g_useAnimation = PATCH_NULL;
static patch_handle_t g_useTime = PATCH_NULL;
static patch_handle_t g_width = PATCH_NULL;
static patch_handle_t g_height = PATCH_NULL;
static patch_handle_t g_shoot = PATCH_NULL;
static patch_handle_t g_useAmmo = PATCH_NULL;
static patch_handle_t g_damage = PATCH_NULL;
static patch_handle_t g_shootSpeed = PATCH_NULL;
static patch_handle_t g_noMelee = PATCH_NULL;
static patch_handle_t g_value = PATCH_NULL;
static patch_handle_t g_ranged = PATCH_NULL;
static patch_handle_t g_channel = PATCH_NULL;
static patch_handle_t g_rare = PATCH_NULL;
static patch_handle_t g_autoReuse = PATCH_NULL;
static patch_handle_t g_mana = PATCH_NULL;
static patch_handle_t g_magic = PATCH_NULL;
static patch_handle_t g_noUseGraphic = PATCH_NULL;

// 方法句柄
static patch_handle_t g_DefaultToBow = PATCH_NULL;
static patch_handle_t g_SetWeaponValues = PATCH_NULL;
static patch_handle_t g_DefaultToFood = PATCH_NULL;
static patch_handle_t g_buyPrice = PATCH_NULL;

static int g_use_2param = 0;

// Hook ID
static patch_hook_id_t g_setdefaults_hook = PATCH_HOOK_INVALID_ID;
static int g_initialized = 0;

// 辅助函数：调用方法
static int call_buyPrice(int platinum, int gold, int silver, int copper, int unknown) {
    (void)unknown;
    if (!g_buyPrice) return 0;

    void* args[4] = { &platinum, &gold, &silver, &copper };
    int result = 0;
    patchlib_method_invoke_args(g_buyPrice, NULL, &result, args);
    return result;
}

static void call_DefaultToBow(patch_handle_t item, int unknown, float speed, int unknown2) {
    if (!g_DefaultToBow) return;
    void* args[3] = { &unknown, &speed, &unknown2 };
    patchlib_method_invoke_args(g_DefaultToBow, item, NULL, args);
}

static void call_SetWeaponValues(patch_handle_t item, int damage, int knockback, float useTime, int unknown) {
    if (!g_SetWeaponValues) return;
    void* args[3] = { &damage, &knockback, &useTime };
    patchlib_method_invoke_args(g_SetWeaponValues, item, NULL, args);
}

static void call_DefaultToFood(patch_handle_t item, int buffType, int buffTime, int useTime, int useAnimation,
                                int useStyle, int rare, int value, int useTurn, int eatSound) {
    if (!g_DefaultToFood) return;
    void* args[6] = { &buffType, &buffTime, &useTime, &useAnimation, &useStyle, &rare };
    patchlib_method_invoke_args(g_DefaultToFood, item, NULL, args);
}

// Hook 函数
static void OnSetDefaults(patch_handle_t item, void** args, void* result,
                          const patch_method_signature_t* sig) {
    (void)result;
    (void)sig;

    if (!item || !args) return;

    int type = *(int*)args[0];

    switch (type) {
        case 2881: { // 火星飞碟
            int val = 5;
            patchlib_field_set_value(g_useStyle, item, &val);
            val = 20;
            patchlib_field_set_value(g_useAnimation, item, &val);
            val = 80;
            patchlib_field_set_value(g_useTime, item, &val);
            float fval = 500.0f;
            patchlib_field_set_value(g_shootSpeed, item, &fval);
            val = 20;
            patchlib_field_set_value(g_width, item, &val);
            val = 12;
            patchlib_field_set_value(g_height, item, &val);
            val = 550;
            patchlib_field_set_value(g_damage, item, &val);
            val = 79;
            patchlib_field_set_value(g_shoot, item, &val);
            val = 100;
            patchlib_field_set_value(g_mana, item, &val);
            val = 8;
            patchlib_field_set_value(g_rare, item, &val);
            int money = call_buyPrice(4, 0, 10, 0, 0);
            patchlib_field_set_value(g_value, item, &money);
            int bval = 1;
            patchlib_field_set_value(g_noMelee, item, &bval);
            patchlib_field_set_value(g_channel, item, &bval);
            patchlib_field_set_value(g_magic, item, &bval);
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Set item 2881");
        } break;

        case 3850: { // 无趣弓变体
            int val = 1;
            patchlib_field_set_value(g_useStyle, item, &val);
            float fval = 55.5f;
            patchlib_field_set_value(g_shootSpeed, item, &fval);
            val = 680;
            patchlib_field_set_value(g_shoot, item, &val);
            val = 71;
            patchlib_field_set_value(g_damage, item, &val);
            val = 0;
            patchlib_field_set_value(g_width, item, &val);
            val = 30;
            patchlib_field_set_value(g_height, item, &val);
            val = 17;
            patchlib_field_set_value(g_useAnimation, item, &val);
            patchlib_field_set_value(g_useTime, item, &val);
            int bval = 1;
            patchlib_field_set_value(g_noUseGraphic, item, &bval);
            patchlib_field_set_value(g_noMelee, item, &bval);
            int money = call_buyPrice(4, 0, 5, 0, 0);
            patchlib_field_set_value(g_value, item, &money);
            patchlib_field_set_value(g_ranged, item, &bval);
            val = 6;
            patchlib_field_set_value(g_rare, item, &val);
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Set item 3850");
        } break;

        case 3853: { // 无趣弓
            int val = 5;
            patchlib_field_set_value(g_useStyle, item, &val);
            val = 25;
            patchlib_field_set_value(g_useAnimation, item, &val);
            val = 40;
            patchlib_field_set_value(g_useAmmo, item, &val);
            val = 20;
            patchlib_field_set_value(g_width, item, &val);
            val = 34;
            patchlib_field_set_value(g_height, item, &val);
            val = 1;
            patchlib_field_set_value(g_shoot, item, &val);
            val = 72;
            patchlib_field_set_value(g_damage, item, &val);
            float fval = 50.0f;
            patchlib_field_set_value(g_shootSpeed, item, &fval);
            int bval = 1;
            patchlib_field_set_value(g_noMelee, item, &bval);
            val = 180000;
            patchlib_field_set_value(g_value, item, &val);
            patchlib_field_set_value(g_ranged, item, &bval);
            val = 12;
            patchlib_field_set_value(g_useTime, item, &val);
            patchlib_field_set_value(g_channel, item, &bval);
            val = 6;
            patchlib_field_set_value(g_rare, item, &val);
            patchlib_field_set_value(g_autoReuse, item, &bval);
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Set item 3853");
        } break;

        case 4058: {
            int val = 40;
            patchlib_field_set_value(g_damage, item, &val);
            call_SetWeaponValues(item, 3, 30, 5.0f, 0);
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Set item 4058");
        } break;

        case 4722: {
            int bval = 0;
            patchlib_field_set_value(g_noMelee, item, &bval);
            int val = 130;
            patchlib_field_set_value(g_damage, item, &val);
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Set item 4722");
        } break;

        case 4010: {
            call_DefaultToFood(item, 6, 22, 22, 207, 17, 14400, 0, 0, 0);
            int val = 3;
            patchlib_field_set_value(g_rare, item, &val);
            int money = call_buyPrice(4, 0, 0, 70, 5);
            patchlib_field_set_value(g_value, item, &money);
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Set item 4010");
        } break;

        default:
            break;
    }
}

// 初始化函数
void item_init(void) {
    if (g_initialized) return;

    // 获取 Item 类型
    patch_handle_t itemType = patchlib_type_get_type("Terraria", "Item");
    if (!itemType) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to get Item type");
        return;
    }

    // 获取字段
    g_useStyle = patchlib_type_get_field(itemType, "useStyle");
    g_useAnimation = patchlib_type_get_field(itemType, "useAnimation");
    g_useTime = patchlib_type_get_field(itemType, "useTime");
    g_width = patchlib_type_get_field(itemType, "width");
    g_height = patchlib_type_get_field(itemType, "height");
    g_shoot = patchlib_type_get_field(itemType, "shoot");
    g_useAmmo = patchlib_type_get_field(itemType, "useAmmo");
    g_damage = patchlib_type_get_field(itemType, "damage");
    g_shootSpeed = patchlib_type_get_field(itemType, "shootSpeed");
    g_noMelee = patchlib_type_get_field(itemType, "noMelee");
    g_value = patchlib_type_get_field(itemType, "value");
    g_ranged = patchlib_type_get_field(itemType, "ranged");
    g_channel = patchlib_type_get_field(itemType, "channel");
    g_rare = patchlib_type_get_field(itemType, "rare");
    g_autoReuse = patchlib_type_get_field(itemType, "autoReuse");
    g_mana = patchlib_type_get_field(itemType, "mana");
    g_magic = patchlib_type_get_field(itemType, "magic");
    g_noUseGraphic = patchlib_type_get_field(itemType, "noUseGraphic");

    // 获取方法
    g_DefaultToBow = patchlib_type_get_method_by_param_count(itemType, "DefaultToBow", 3);
    g_SetWeaponValues = patchlib_type_get_method_by_param_count(itemType, "SetWeaponValues", 3);
    g_DefaultToFood = patchlib_type_get_method_by_param_count(itemType, "DefaultToFood", 6);
    g_buyPrice = patchlib_type_get_method_by_param_count(itemType, "buyPrice", 4);

    // 尝试获取 SetDefaults 方法 - 优先使用 2 参数版本
    patch_handle_t setdefaults = patchlib_type_get_method_by_param_count(itemType, "SetDefaults", 2);
    if (!setdefaults) {
        // 回退到 3 参数版本
        setdefaults = patchlib_type_get_method_by_param_count(itemType, "SetDefaults", 3);
        g_use_2param = 0;
        mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "Using SetDefaults with 3 parameters");
    } else {
        g_use_2param = 1;
        mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "Using SetDefaults with 2 parameters");
    }

    if (setdefaults) {
        g_setdefaults_hook = patchlib_install_prepost_hook(setdefaults, NULL, OnSetDefaults);
        patchlib_free(setdefaults);
    }

    patchlib_free(itemType);
    g_initialized = 1;
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "Item initialized");
}

// 清理函数
void item_cleanup(void) {
    if (g_setdefaults_hook != PATCH_HOOK_INVALID_ID) {
        patchlib_uninstall_hook(g_setdefaults_hook);
    }
    if (g_useStyle) patchlib_free(g_useStyle);
    if (g_useAnimation) patchlib_free(g_useAnimation);
    if (g_useTime) patchlib_free(g_useTime);
    if (g_width) patchlib_free(g_width);
    if (g_height) patchlib_free(g_height);
    if (g_shoot) patchlib_free(g_shoot);
    if (g_useAmmo) patchlib_free(g_useAmmo);
    if (g_damage) patchlib_free(g_damage);
    if (g_shootSpeed) patchlib_free(g_shootSpeed);
    if (g_noMelee) patchlib_free(g_noMelee);
    if (g_value) patchlib_free(g_value);
    if (g_ranged) patchlib_free(g_ranged);
    if (g_channel) patchlib_free(g_channel);
    if (g_rare) patchlib_free(g_rare);
    if (g_autoReuse) patchlib_free(g_autoReuse);
    if (g_mana) patchlib_free(g_mana);
    if (g_magic) patchlib_free(g_magic);
    if (g_noUseGraphic) patchlib_free(g_noUseGraphic);
    if (g_DefaultToBow) patchlib_free(g_DefaultToBow);
    if (g_SetWeaponValues) patchlib_free(g_SetWeaponValues);
    if (g_DefaultToFood) patchlib_free(g_DefaultToFood);
    if (g_buyPrice) patchlib_free(g_buyPrice);
    g_initialized = 0;
}