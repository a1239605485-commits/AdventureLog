/*******************************************************************************
 * File: mod
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

#include <stddef.h>

#include "mod_core.h"
#include "mod_logger.h"

void (*mod_logger_write)(mod_log_level_t level, const char* tag, const char* fmt, ...) = NULL;

void item_id_init(void);
void item_id_cleanup(void);

void item_init(void);
void item_cleanup(void);

void item_drop_database_init(void);
void item_drop_database_cleanup(void);

void prefix_legacy_init(void);
void prefix_legacy_cleanup(void);

void recipe_init(void);
void recipe_cleanup(void);

static void init_mod(kernel_mod_handle_t* handle) {
    (void)handle;
    item_id_init();
    // item_init();
    // item_drop_database_init();
    // prefix_legacy_init();
    recipe_init();
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "Loaded");
}

static void cleanup_mod(kernel_mod_handle_t* handle) {
    (void)handle;
    item_id_cleanup();
    // item_cleanup();
    // item_drop_database_cleanup();
    // prefix_legacy_cleanup();
    recipe_cleanup();
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ForgottenItem", "Unloaded");
}

static kernel_mod_info_t g_info = {
    .pkg_id = "eternal.future.forgottenitem",
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
