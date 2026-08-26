#include <stdbool.h>
#include <stddef.h>

#include "mod_core.h"
#include "mod_logger.h"
#include "tefkernel/patchlib/method.h"
#include "tefkernel/patchlib/type.h"

void (*mod_logger_write)(mod_log_level_t level, const char *tag,
                         const char *fmt, ...) = NULL;

static patch_hook_id_t g_grow_tree_hook = PATCH_HOOK_INVALID_ID;
static patch_hook_id_t g_grow_palm_hook = PATCH_HOOK_INVALID_ID;
static patch_hook_id_t g_shake_tree_hook = PATCH_HOOK_INVALID_ID;
static patch_hook_id_t g_random_next_one_hook = PATCH_HOOK_INVALID_ID;
static patch_hook_id_t g_random_next_two_hook = PATCH_HOOK_INVALID_ID;

/* Terraria executes these methods on the main game thread. Counters are used
   instead of booleans so nested calls remain scoped correctly. */
static int g_growth_random_scope = 0;
static int g_shake_random_scope = 0;

static bool growth_prefix(patch_handle_t instance, void **args,
                          const patch_method_signature_t *sig, void *result) {
    (void)instance; (void)args; (void)sig; (void)result;
    ++g_growth_random_scope;
    return true;
}

static void growth_postfix(patch_handle_t instance, void **args, void *result,
                           const patch_method_signature_t *sig) {
    (void)instance; (void)args; (void)result; (void)sig;
    if (g_growth_random_scope > 0) --g_growth_random_scope;
}

static bool shake_prefix(patch_handle_t instance, void **args,
                         const patch_method_signature_t *sig, void *result) {
    (void)instance; (void)args; (void)sig; (void)result;
    ++g_shake_random_scope;
    return true;
}

static void shake_postfix(patch_handle_t instance, void **args, void *result,
                          const patch_method_signature_t *sig) {
    (void)instance; (void)args; (void)result; (void)sig;
    if (g_shake_random_scope > 0) --g_shake_random_scope;
}

static void random_next_one_postfix(patch_handle_t instance, void **args,
                                    void *result,
                                    const patch_method_signature_t *sig) {
    (void)instance; (void)args; (void)sig;
    if (result && (g_growth_random_scope > 0 || g_shake_random_scope > 0)) {
        *(int *)result = 0;
    }
}

static void random_next_two_postfix(patch_handle_t instance, void **args,
                                    void *result,
                                    const patch_method_signature_t *sig) {
    (void)instance; (void)sig;
    if (result && args && args[0] &&
        (g_growth_random_scope > 0 || g_shake_random_scope > 0)) {
        /* Random.Next(minValue, maxValue) must return a value in range. */
        *(int *)result = *(int *)args[0];
    }
}

static patch_hook_id_t hook_scoped_method(patch_handle_t type,
                                           const char *name,
                                           prefix_callback_t prefix,
                                           postfix_callback_t postfix) {
    patch_handle_t method = patchlib_type_get_method_by_param_count(type, name, 2);
    if (!method) return PATCH_HOOK_INVALID_ID;
    patch_hook_id_t id = patchlib_install_prepost_hook(method, prefix, postfix);
    patchlib_free(method);
    return id;
}

static void init_mod(kernel_mod_handle_t *handle) {
    (void)handle;
    patch_handle_t world_gen = patchlib_type_get_type("Terraria", "WorldGen");
    patch_handle_t random = patchlib_type_get_type("Terraria.Utilities", "UnifiedRandom");
    if (!world_gen || !random) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "TreeBlessing",
                         "WorldGen or UnifiedRandom type not found");
        if (world_gen) patchlib_free(world_gen);
        if (random) patchlib_free(random);
        return;
    }

    g_grow_tree_hook = hook_scoped_method(world_gen, "GrowTree",
                                           growth_prefix, growth_postfix);
    g_grow_palm_hook = hook_scoped_method(world_gen, "GrowPalmTree",
                                           growth_prefix, growth_postfix);
    g_shake_tree_hook = hook_scoped_method(world_gen, "ShakeTree",
                                            shake_prefix, shake_postfix);

    patch_handle_t next_one = patchlib_type_get_method_by_param_count(random, "Next", 1);
    patch_handle_t next_two = patchlib_type_get_method_by_param_count(random, "Next", 2);
    if (next_one) {
        g_random_next_one_hook = patchlib_install_prepost_hook(
            next_one, NULL, random_next_one_postfix);
        patchlib_free(next_one);
    }
    if (next_two) {
        g_random_next_two_hook = patchlib_install_prepost_hook(
            next_two, NULL, random_next_two_postfix);
        patchlib_free(next_two);
    }

    patchlib_free(world_gen);
    patchlib_free(random);

    if (g_grow_tree_hook == PATCH_HOOK_INVALID_ID ||
        g_grow_palm_hook == PATCH_HOOK_INVALID_ID ||
        g_shake_tree_hook == PATCH_HOOK_INVALID_ID ||
        g_random_next_one_hook == PATCH_HOOK_INVALID_ID) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "TreeBlessing",
                         "One or more required hooks failed");
    } else {
        mod_logger_write(MOD_LOG_LEVEL_INFO, "TreeBlessing",
                         "Fast growth and guaranteed tree drops enabled");
    }
}

static void uninstall(patch_hook_id_t *id) {
    if (*id != PATCH_HOOK_INVALID_ID) {
        patchlib_uninstall_hook(*id);
        *id = PATCH_HOOK_INVALID_ID;
    }
}

static void cleanup_mod(kernel_mod_handle_t *handle) {
    (void)handle;
    uninstall(&g_random_next_two_hook);
    uninstall(&g_random_next_one_hook);
    uninstall(&g_shake_tree_hook);
    uninstall(&g_grow_palm_hook);
    uninstall(&g_grow_tree_hook);
    g_growth_random_scope = 0;
    g_shake_random_scope = 0;
    mod_logger_write(MOD_LOG_LEVEL_INFO, "TreeBlessing", "Unloaded");
}

static kernel_mod_info_t g_info = {
    .pkg_id = "lzup.gameplay.treeblessing",
    .version_code = 2026082501,
    .api_version = 1,
    .version = "1.0.0-test"
};

static kernel_mod_info_t *get_info(void) { return &g_info; }

static kernel_mod_ops_t g_ops = {
    .init_mod = init_mod,
    .cleanup_mod = cleanup_mod,
    .get_info = get_info
};

kernel_mod_ops_t *create_kernel_mod(void) { return &g_ops; }
