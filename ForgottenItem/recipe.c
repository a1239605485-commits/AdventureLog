/*******************************************************************************
 * File: recipe
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

// 字段句柄
static patch_handle_t g_createItem = PATCH_NULL;
static patch_handle_t g_currentRecipe = PATCH_NULL;
static patch_handle_t g_requiredItem = PATCH_NULL;      // Item[] 数组字段
static patch_handle_t g_requiredTile = PATCH_NULL;      // int 字段
static patch_handle_t g_stack = PATCH_NULL;
static patch_handle_t g_IsAMaterial = PATCH_NULL;

// 方法句柄
static patch_handle_t g_AddRecipe = PATCH_NULL;
static patch_handle_t g_ItemSetDefaults = PATCH_NULL;

// Hook ID
static patch_hook_id_t g_setup_hook = PATCH_HOOK_INVALID_ID;
static int g_initialized = 0;
static int g_recipes_added = 0;

// 材料集合
static int g_materialItems[256];
static int g_materialCount = 0;

// 配方配置结构体
typedef struct {
    int resultItemID;
    int resultStack;
    int materials[8][2];
    int materialCount;
    int requiredTileID;
} RecipeConfig;

// 添加材料到集合
static void add_material(int itemID) {
    for (int i = 0; i < g_materialCount; i++) {
        if (g_materialItems[i] == itemID) return;
    }
    if (g_materialCount < 256) {
        g_materialItems[g_materialCount++] = itemID;
    }
}

// 检查句柄是否有效
static int is_valid_handle(patch_handle_t handle, const char* name) {
    if (!handle || handle == PATCH_NULL || handle == (patch_handle_t)0xffffffff) {
        mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "%s is invalid: %p", name, handle);
        return 0;
    }
    return 1;
}

// 检查数组元素是否有效
static int is_valid_slot(patch_handle_t slot, int index) {
    if (!slot || slot == PATCH_NULL || slot == (patch_handle_t)0xffffffff) {
        mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "materialSlot[%d] is invalid: %p", index, slot);
        return 0;
    }
    return 1;
}

// 添加单个配方组
static void add_recipe_group(RecipeConfig* config) {
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "=== add_recipe_group start for item %d ===", config->resultItemID);
    
    // 检查字段句柄
    if (!g_currentRecipe || !g_requiredItem || !g_createItem || !g_requiredTile || !g_stack) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Recipe fields not initialized");
        return;
    }

    // 获取 currentRecipe 实例
    patch_handle_t recipePtr = PATCH_NULL;
    patchlib_field_get_value(g_currentRecipe, NULL, &recipePtr);
    
    if (!is_valid_handle(recipePtr, "recipePtr")) {
        return;
    }
    mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "recipePtr = %p", recipePtr);

    // 获取 requiredItem 数组（g_requiredItem 是数组字段，直接获取数组对象）
    patch_handle_t requiredItemArray = PATCH_NULL;
    patchlib_field_get_value(g_requiredItem, recipePtr, &requiredItemArray);
    if (!is_valid_handle(requiredItemArray, "requiredItemArray")) {
        return;
    }
    mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "requiredItemArray = %p", requiredItemArray);

    // 获取数组大小
    size_t arraySize = patchlib_array_length(requiredItemArray);
    mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "requiredItemArray size = %d", arraySize);

    // 获取 createItem 实例
    patch_handle_t createItem_instance = PATCH_NULL;
    patchlib_field_get_value(g_createItem, recipePtr, &createItem_instance);
    if (!is_valid_handle(createItem_instance, "createItem_instance")) {
        return;
    }
    mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "createItem_instance = %p", createItem_instance);

    // requiredTile 是 int 字段，直接获取值（用于调试）
    int requiredTileValue = 0;
    patchlib_field_get_value(g_requiredTile, recipePtr, &requiredTileValue);
    mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "current requiredTile value = %d", requiredTileValue);

    // 设置产出物品
    if (createItem_instance && g_ItemSetDefaults) {
        int stackVal = config->resultStack > 0 ? config->resultStack : 1;
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Setting result item: ID=%d, stack=%d", 
                         config->resultItemID, stackVal);

        // 调用 SetDefaults 设置物品类型
        void* args[2] = { &config->resultItemID, NULL };
        if (patchlib_method_invoke_args(g_ItemSetDefaults, createItem_instance, NULL, args) != 0) {
            mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Result item SetDefaults failed");
        } else {
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Result item SetDefaults completed");
        }

        // 设置堆叠数量
        patchlib_field_set_value(g_stack, createItem_instance, &stackVal);
    }

    // 设置材料
    mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Setting up %d materials", config->materialCount);
    for (int i = 0; i < config->materialCount && i < 8; i++) {
        int itemID = config->materials[i][0];
        int stackSize = config->materials[i][1];
        
        add_material(itemID);

        // 检查数组索引是否越界
        if (i >= arraySize) {
            mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "Index %d out of bounds (size=%d)", i, arraySize);
            continue;
        }

        // 获取数组元素（Item 对象）
        patch_handle_t materialSlot = PATCH_NULL;
        if (patchlib_array_at(requiredItemArray, i, &materialSlot) != 0) {
            mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "Failed to get materialSlot[%d]", i);
            continue;
        }
        
        if (!is_valid_slot(materialSlot, i)) {
            continue;
        }
        
        if (g_ItemSetDefaults) {
            // 设置材料类型
            void* args[2] = { &itemID, NULL };
            if (patchlib_method_invoke_args(g_ItemSetDefaults, materialSlot, NULL, args) != 0) {
                mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Material[%d] SetDefaults failed", i);
                continue;
            }
            
            // 设置堆叠数量（如果大于1）
            if (stackSize > 1) {
                patchlib_field_set_value(g_stack, materialSlot, &stackSize);
            }
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Material[%d] set: ID=%d, stack=%d", i, itemID, stackSize);
        }
    }

    // 设置工作台 - requiredTile 是 int 字段，直接设置值
    if (config->requiredTileID > 0) {
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Setting requiredTile to %d (was %d)", 
                         config->requiredTileID, requiredTileValue);
        patchlib_field_set_value(g_requiredTile, recipePtr, &config->requiredTileID);
    }

    // 提交配方
    if (g_AddRecipe) {
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Calling AddRecipe");
        if (patchlib_method_invoke_args(g_AddRecipe, NULL, NULL, NULL) != 0) {
            mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "AddRecipe failed");
        } else {
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "AddRecipe completed");
        }
    }
    
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "=== add_recipe_group completed for item %d ===", config->resultItemID);
}

// 设置配方组
static void setup_recipe_groups() {
    if (g_recipes_added) {
        mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "Recipes already added");
        return;
    }
    
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "Adding custom recipes...");
    
    // 发光工具组
    int luminousTools[4][4] = {
        {3458, 2785, 2783, 2782},  // 镐
        {3456, 2775, 2773, 2772},  // 斧
        {3457, 2780, 2778, 2777},  // 锤
        {3459, 3465, 3463, 3462}   // 剑
    };

    for (int i = 0; i < 4; i++) {
        RecipeConfig config = {
            .resultItemID = luminousTools[i][0],
            .resultStack = 1,
            .materialCount = 2,
            .requiredTileID = 412  // 恶魔祭坛
        };
        config.materials[0][0] = luminousTools[i][1];
        config.materials[0][1] = 1;
        config.materials[1][0] = 3467;
        config.materials[1][1] = 6;
        add_recipe_group(&config);
    }

    // 旧礼物配方
    int giftIDs[] = {599, 600, 601};
    for (int i = 0; i < 3; i++) {
        RecipeConfig config = {
            .resultItemID = giftIDs[i],
            .resultStack = 1,
            .materialCount = 1,
            .requiredTileID = 0
        };
        config.materials[0][0] = 1869;
        config.materials[0][1] = 1;
        add_recipe_group(&config);
    }

    // 骷髅头弓
    RecipeConfig bow = {
        .resultItemID = 4058,
        .resultStack = 1,
        .materialCount = 2,
        .requiredTileID = 18  // 铁砧
    };
    bow.materials[0][0] = 1274;
    bow.materials[0][1] = 1;
    bow.materials[1][0] = 154;
    bow.materials[1][1] = 50;
    add_recipe_group(&bow);

    // 食物配方
    RecipeConfig food1 = {
        .resultItemID = 4010,
        .resultStack = 4,
        .materialCount = 1,
        .requiredTileID = 0
    };
    food1.materials[0][0] = 4011;
    food1.materials[0][1] = 1;
    add_recipe_group(&food1);

    RecipeConfig food2 = {
        .resultItemID = 4011,
        .resultStack = 1,
        .materialCount = 1,
        .requiredTileID = 0
    };
    food2.materials[0][0] = 4010;
    food2.materials[0][1] = 4;
    add_recipe_group(&food2);

    // 分形
    RecipeConfig fractal = {
        .resultItemID = 4722,
        .resultStack = 1,
        .materialCount = 5,
        .requiredTileID = 134  // 远古操纵机
    };
    fractal.materials[0][0] = 757;
    fractal.materials[0][1] = 1;
    fractal.materials[1][0] = 3827;
    fractal.materials[1][1] = 1;
    fractal.materials[2][0] = 3787;
    fractal.materials[2][1] = 1;
    fractal.materials[3][0] = 1570;
    fractal.materials[3][1] = 2;
    fractal.materials[4][0] = 2880;
    fractal.materials[4][1] = 1;
    add_recipe_group(&fractal);

    // 骨头
    RecipeConfig bone = {
        .resultItemID = 766,
        .resultStack = 1,
        .materialCount = 1,
        .requiredTileID = 18
    };
    bone.materials[0][0] = 154;
    bone.materials[0][1] = 1;
    add_recipe_group(&bone);

    // 假箱子
    RecipeConfig fakeChest1 = {
        .resultItemID = 3705,
        .resultStack = 1,
        .materialCount = 1,
        .requiredTileID = 0
    };
    fakeChest1.materials[0][0] = 3886;
    fakeChest1.materials[0][1] = 1;
    add_recipe_group(&fakeChest1);

    RecipeConfig fakeChest2 = {
        .resultItemID = 3706,
        .resultStack = 1,
        .materialCount = 1,
        .requiredTileID = 0
    };
    fakeChest2.materials[0][0] = 3887;
    fakeChest2.materials[0][1] = 1;
    add_recipe_group(&fakeChest2);

    // 睡眠图标
    RecipeConfig sleepIcon = {
        .resultItemID = 5013,
        .resultStack = 1,
        .materialCount = 0,
        .requiredTileID = 79  // 工作台
    };
    add_recipe_group(&sleepIcon);

    // 设置材料属性（让这些物品在材料栏可见）
    if (g_IsAMaterial && g_materialCount > 0) {
        patch_handle_t isMaterialArray = PATCH_NULL;
        patchlib_field_get_value(g_IsAMaterial, NULL, &isMaterialArray);
        if (is_valid_handle(isMaterialArray, "isMaterialArray")) {
            for (int i = 0; i < g_materialCount; i++) {
                int val = 1;
                patchlib_array_set(isMaterialArray, g_materialItems[i], &val);
            }
            mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "Set IsAMaterial for %d items", g_materialCount);
        }
    }

    g_recipes_added = 1;
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "All recipes added (%d material items)", g_materialCount);
}

// Post Hook 函数（在原始 SetupRecipes 执行后调用）
static void OnSetupRecipeGroups_Post(patch_handle_t instance, void** args, void* result,
                                      const patch_method_signature_t* sig) {
    (void)args;
    (void)result;
    (void)sig;
    (void)instance;

    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "SetupRecipes Post Hook - adding custom recipes");
    setup_recipe_groups();
}

// 初始化函数
void recipe_init(void) {
    if (g_initialized) {
        mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "Recipe already initialized");
        return;
    }

    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "Initializing recipe system...");

    // 获取 Recipe 类型
    patch_handle_t recipeType = patchlib_type_get_type("Terraria", "Recipe");
    if (!recipeType) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to get Recipe type");
        return;
    }

    // 获取字段
    g_createItem = patchlib_type_get_field(recipeType, "createItem");
    g_currentRecipe = patchlib_type_get_field(recipeType, "currentRecipe");
    g_requiredItem = patchlib_type_get_field(recipeType, "requiredItem");   // Item[] 数组
    g_requiredTile = patchlib_type_get_field(recipeType, "requiredTile");   // int 字段

    if (!g_createItem || !g_currentRecipe || !g_requiredItem || !g_requiredTile) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to get required fields");
        patchlib_free(recipeType);
        return;
    }
    mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "Fields obtained: createItem=%p, currentRecipe=%p, requiredItem=%p, requiredTile=%p",
                     g_createItem, g_currentRecipe, g_requiredItem, g_requiredTile);

    // 获取 Item 类型和字段
    patch_handle_t itemType = patchlib_type_get_type("Terraria", "Item");
    if (itemType) {
        g_stack = patchlib_type_get_field(itemType, "stack");
        if (!g_stack) {
            mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "Failed to get Item.stack field");
        } else {
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "stack field obtained: %p", g_stack);
        }
    }

    // 获取方法
    g_AddRecipe = patchlib_type_get_method_by_param_count(recipeType, "AddRecipe", 0);
    if (!g_AddRecipe) {
        mod_logger_write(MOD_LOG_LEVEL_WARNING, "ForgottenItem", "Failed to get Recipe.AddRecipe method");
    } else {
        mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "AddRecipe method obtained: %p", g_AddRecipe);
    }
    
    if (itemType) {
        g_ItemSetDefaults = patchlib_type_get_method_by_param_count(itemType, "SetDefaults", 2);
        if (!g_ItemSetDefaults) {
            g_ItemSetDefaults = patchlib_type_get_method_by_param_count(itemType, "SetDefaults", 1);
        }
        if (!g_ItemSetDefaults) {
            mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to get Item.SetDefaults method");
        } else {
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "SetDefaults method obtained: %p", g_ItemSetDefaults);
        }
    }

    // 获取 IsAMaterial 字段
    patch_handle_t itemIdType = patchlib_type_get_type("Terraria.ID", "ItemID");
    if (itemIdType) {
        patch_handle_t setsType = patchlib_type_get_inner_type(itemIdType, "Sets");
        if (setsType) {
            g_IsAMaterial = patchlib_type_get_field(setsType, "IsAMaterial");
            patchlib_free(setsType);
            mod_logger_write(MOD_LOG_LEVEL_DEBUG, "ForgottenItem", "IsAMaterial field obtained: %p", g_IsAMaterial);
        }
        patchlib_free(itemIdType);
    }

    // 安装 Post Hook（在原始方法执行后调用）
    patch_handle_t setupMethod = patchlib_type_get_method_by_param_count(recipeType, "SetupRecipes", 0);
    if (setupMethod) {
        g_setup_hook = patchlib_install_prepost_hook(setupMethod, NULL, OnSetupRecipeGroups_Post);
        if (g_setup_hook != PATCH_HOOK_INVALID_ID) {
            mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "Post hook installed successfully on SetupRecipes");
        } else {
            mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to install hook on SetupRecipes");
        }
        patchlib_free(setupMethod);
    } else {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ForgottenItem", "Failed to get SetupRecipes method");
    }

    patchlib_free(recipeType);
    if (itemType) patchlib_free(itemType);
    
    g_initialized = 1;
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "Recipe system initialized");
}

// 清理函数
void recipe_cleanup(void) {
    if (g_setup_hook != PATCH_HOOK_INVALID_ID) {
        patchlib_uninstall_hook(g_setup_hook);
        g_setup_hook = PATCH_HOOK_INVALID_ID;
    }
    
    if (g_createItem) patchlib_free(g_createItem);
    if (g_currentRecipe) patchlib_free(g_currentRecipe);
    if (g_requiredItem) patchlib_free(g_requiredItem);
    if (g_requiredTile) patchlib_free(g_requiredTile);
    if (g_stack) patchlib_free(g_stack);
    if (g_IsAMaterial) patchlib_free(g_IsAMaterial);
    if (g_AddRecipe) patchlib_free(g_AddRecipe);
    if (g_ItemSetDefaults) patchlib_free(g_ItemSetDefaults);

    g_materialCount = 0;
    g_initialized = 0;
    g_recipes_added = 0;
    
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "Recipe system cleaned up");
}