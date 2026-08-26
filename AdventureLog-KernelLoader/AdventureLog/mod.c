#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "mod_core.h"
#include "mod_logger.h"
#include "tefkernel/patchlib/field.h"
#include "tefkernel/patchlib/method.h"
#include "tefkernel/patchlib/type.h"

void (*mod_logger_write)(mod_log_level_t level, const char *tag,
                         const char *fmt, ...) = NULL;

enum { PLAYER_SLOTS = 256, CELL_WIDTH = 1200, CELL_HEIGHT = 720 };

typedef struct { float x; float y; } vector2_t;

static patch_handle_t g_player_dead = PATCH_NULL;
static patch_handle_t g_player_id = PATCH_NULL;
static patch_handle_t g_player_position = PATCH_NULL;
static patch_handle_t g_npc_type = PATCH_NULL;
static patch_handle_t g_npc_boss = PATCH_NULL;
static patch_handle_t g_item_type = PATCH_NULL;
static patch_handle_t g_item_stack = PATCH_NULL;
static patch_handle_t g_item_rare = PATCH_NULL;
static patch_handle_t g_main_day_time = PATCH_NULL;

static patch_hook_id_t g_player_update_hook = PATCH_HOOK_INVALID_ID;
static patch_hook_id_t g_npc_loot_hook = PATCH_HOOK_INVALID_ID;
static patch_hook_id_t g_pickup_hook = PATCH_HOOK_INVALID_ID;

static bool g_was_dead[PLAYER_SLOTS];
static int g_last_cell_x[PLAYER_SLOTS];
static int g_last_cell_y[PLAYER_SLOTS];
static bool g_has_cell[PLAYER_SLOTS];
static bool g_last_day_time;
static bool g_day_state_ready;
static unsigned g_day_number;
static char g_log_path[1024];

static void write_log(const char *fmt, ...) {
    char line[768];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    if (mod_logger_write)
        mod_logger_write(MOD_LOG_LEVEL_INFO, "AdventureLog", "%s", line);

    if (!g_log_path[0]) return;
    FILE *file = fopen(g_log_path, "a");
    if (!file) return;
    fputs(line, file);
    fputc('\n', file);
    fclose(file);
}

static int valid_player_id(int id) { return id >= 0 && id < PLAYER_SLOTS; }

/* Player.Update is intentionally used as the single low-overhead observation
   point for deaths, respawns, exploration and day changes. It never changes
   game state; it only reads KernelLoader handles. */
static void player_update_postfix(patch_handle_t player, void **args, void *result,
                                  const patch_method_signature_t *sig) {
    (void)args; (void)result; (void)sig;
    if (!player) return;

    int id = -1;
    bool dead = false;
    vector2_t position = {0.0f, 0.0f};
    patchlib_field_get_value(g_player_id, player, &id);
    patchlib_field_get_value(g_player_dead, player, &dead);
    if (!valid_player_id(id)) return;

    if (dead && !g_was_dead[id]) write_log("[Death] player=%d", id);
    if (!dead && g_was_dead[id]) write_log("[Respawn] player=%d", id);
    g_was_dead[id] = dead;

    if (!dead && g_player_position) {
        patchlib_field_get_value(g_player_position, player, &position);
        int cx = (int)(position.x / CELL_WIDTH);
        int cy = (int)(position.y / CELL_HEIGHT);
        if (!g_has_cell[id] || cx != g_last_cell_x[id] || cy != g_last_cell_y[id]) {
            write_log("[Explore] player=%d sector=(%d,%d) position=(%.0f,%.0f)",
                      id, cx, cy, position.x, position.y);
            g_last_cell_x[id] = cx;
            g_last_cell_y[id] = cy;
            g_has_cell[id] = true;
        }
    }

    if (g_main_day_time) {
        bool day_time = false;
        patchlib_field_get_value(g_main_day_time, PATCH_NULL, &day_time);
        if (g_day_state_ready && day_time && !g_last_day_time) {
            ++g_day_number;
            write_log("[Day] day=%u", g_day_number);
        }
        g_last_day_time = day_time;
        g_day_state_ready = true;
    }
}

/* NPCLoot is called when Terraria resolves a killed NPC's drops. */
static void npc_loot_postfix(patch_handle_t npc, void **args, void *result,
                             const patch_method_signature_t *sig) {
    (void)args; (void)result; (void)sig;
    if (!npc) return;
    int type = 0;
    bool boss = false;
    patchlib_field_get_value(g_npc_type, npc, &type);
    patchlib_field_get_value(g_npc_boss, npc, &boss);
    write_log(boss ? "[BossKill] npcType=%d" : "[Kill] npcType=%d", type);
}

/* Terraria exposes GetItem in different overloads across supported 1.4 builds.
   The item parameter is slot 0 for an instance overload and slot 1 for a static
   overload with player id first. The signature tells us which layout is active. */
static void get_item_postfix(patch_handle_t player, void **args, void *result,
                             const patch_method_signature_t *sig) {
    (void)result;
    if (!args || !sig) return;
    int item_arg = sig->is_instance ? 0 : 1;
    if (!args[item_arg]) return;
    patch_handle_t item = (patch_handle_t)args[item_arg];
    int type = 0, stack = 0, rare = 0;
    patchlib_field_get_value(g_item_type, item, &type);
    patchlib_field_get_value(g_item_stack, item, &stack);
    patchlib_field_get_value(g_item_rare, item, &rare);
    write_log(rare > 0 ? "[RareItem] type=%d stack=%d rare=%d" :
                         "[Item] type=%d stack=%d rare=%d", type, stack, rare);
    (void)player;
}

static void uninstall(patch_hook_id_t *hook) {
    if (*hook != PATCH_HOOK_INVALID_ID) {
        patchlib_uninstall_hook(*hook);
        *hook = PATCH_HOOK_INVALID_ID;
    }
}

static patch_hook_id_t hook_first(patch_handle_t type, const char *name,
                                  const int *counts, size_t count,
                                  postfix_callback_t callback) {
    for (size_t i = 0; i < count; ++i) {
        patch_handle_t method = patchlib_type_get_method_by_param_count(type, name, counts[i]);
        if (!method) continue;
        patch_hook_id_t hook = patchlib_install_prepost_hook(method, NULL, callback);
        patchlib_free(method);
        if (hook != PATCH_HOOK_INVALID_ID) return hook;
    }
    return PATCH_HOOK_INVALID_ID;
}

static void init_mod(kernel_mod_handle_t *handle) {
    if (!handle || !handle->private_dir) return;
    snprintf(g_log_path, sizeof(g_log_path), "%s/AdventureLog.txt", handle->private_dir);
    write_log("=== AdventureLog started ===");

    patch_handle_t player = patchlib_type_get_type("Terraria", "Player");
    patch_handle_t npc = patchlib_type_get_type("Terraria", "NPC");
    patch_handle_t item = patchlib_type_get_type("Terraria", "Item");
    patch_handle_t main = patchlib_type_get_type("Terraria", "Main");
    if (!player || !npc || !item || !main) {
        write_log("[Error] Terraria type lookup failed");
        if (player) patchlib_free(player); if (npc) patchlib_free(npc);
        if (item) patchlib_free(item); if (main) patchlib_free(main);
        return;
    }

    g_player_dead = patchlib_type_get_field(player, "dead");
    g_player_id = patchlib_type_get_field(player, "whoAmI");
    g_player_position = patchlib_type_get_field(player, "position");
    g_npc_type = patchlib_type_get_field(npc, "type");
    g_npc_boss = patchlib_type_get_field(npc, "boss");
    g_item_type = patchlib_type_get_field(item, "type");
    g_item_stack = patchlib_type_get_field(item, "stack");
    g_item_rare = patchlib_type_get_field(item, "rare");
    g_main_day_time = patchlib_type_get_field(main, "dayTime");

    const int player_update_counts[] = {1, 0};
    const int npc_loot_counts[] = {0};
    const int pickup_counts[] = {3, 4, 2};
    if (g_player_dead && g_player_id && g_player_position && g_main_day_time)
        g_player_update_hook = hook_first(player, "Update", player_update_counts, 2, player_update_postfix);
    if (g_npc_type && g_npc_boss)
        g_npc_loot_hook = hook_first(npc, "NPCLoot", npc_loot_counts, 1, npc_loot_postfix);
    if (g_item_type && g_item_stack && g_item_rare)
        g_pickup_hook = hook_first(player, "GetItem", pickup_counts, 3, get_item_postfix);

    patchlib_free(player); patchlib_free(npc); patchlib_free(item); patchlib_free(main);
    write_log("[Status] player=%s npc=%s pickup=%s",
              g_player_update_hook != PATCH_HOOK_INVALID_ID ? "ok" : "missing",
              g_npc_loot_hook != PATCH_HOOK_INVALID_ID ? "ok" : "missing",
              g_pickup_hook != PATCH_HOOK_INVALID_ID ? "ok" : "missing");
}

static void cleanup_mod(kernel_mod_handle_t *handle) {
    (void)handle;
    uninstall(&g_pickup_hook); uninstall(&g_npc_loot_hook); uninstall(&g_player_update_hook);
    if (g_player_dead) patchlib_free(g_player_dead); if (g_player_id) patchlib_free(g_player_id);
    if (g_player_position) patchlib_free(g_player_position); if (g_npc_type) patchlib_free(g_npc_type);
    if (g_npc_boss) patchlib_free(g_npc_boss); if (g_item_type) patchlib_free(g_item_type);
    if (g_item_stack) patchlib_free(g_item_stack); if (g_item_rare) patchlib_free(g_item_rare);
    if (g_main_day_time) patchlib_free(g_main_day_time);
    g_player_dead = g_player_id = g_player_position = PATCH_NULL;
    g_npc_type = g_npc_boss = g_item_type = g_item_stack = g_item_rare = PATCH_NULL;
    g_main_day_time = PATCH_NULL;
    write_log("=== AdventureLog stopped ===");
}

static kernel_mod_info_t g_info = {
    .pkg_id = "lzup.gameplay.adventurelog", .version_code = 2026082602,
    .api_version = 1, .version = "1.0.0-test"
};
static kernel_mod_info_t *get_info(void) { return &g_info; }
static kernel_mod_ops_t g_ops = { .init_mod = init_mod, .cleanup_mod = cleanup_mod, .get_info = get_info };
kernel_mod_ops_t *create_kernel_mod(void) { return &g_ops; }
