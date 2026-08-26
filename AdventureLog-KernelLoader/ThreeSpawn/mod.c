#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>
#include <unistd.h>

#include "mod_core.h"
#include "mod_logger.h"
#include "tefkernel/patchlib/field.h"
#include "tefkernel/patchlib/type.h"

void (*mod_logger_write)(mod_log_level_t level, const char *tag,
                         const char *fmt, ...) = NULL;

enum {
    TARGET_SPAWN_RATE = 200,
    TARGET_MAX_SPAWNS = 15,
    POLL_INTERVAL_US = 100000
};

static patch_handle_t g_spawn_rate = PATCH_NULL;
static patch_handle_t g_max_spawns = PATCH_NULL;
static pthread_t g_worker;
static volatile bool g_running = false;
static bool g_thread_started = false;
static bool g_base_saved = false;
static int g_base_spawn_rate = 0;
static int g_base_max_spawns = 0;

static void write_targets(void) {
    int rate = TARGET_SPAWN_RATE;
    int maximum = TARGET_MAX_SPAWNS;
    patchlib_field_set_value(g_spawn_rate, PATCH_NULL, &rate);
    patchlib_field_set_value(g_max_spawns, PATCH_NULL, &maximum);
}

static void *monitor_spawn_settings(void *unused) {
    (void)unused;
    while (g_running) {
        int rate = 0;
        int maximum = 0;
        patchlib_field_get_value(g_spawn_rate, PATCH_NULL, &rate);
        patchlib_field_get_value(g_max_spawns, PATCH_NULL, &maximum);

        /* Save only the first valid pair. Never reinterpret our own modified
           values, or a partially updated pair, as a new base value. */
        if (!g_base_saved && rate > 0 && maximum > 0) {
            g_base_spawn_rate = rate;
            g_base_max_spawns = maximum;
            g_base_saved = true;
        }

        if (rate != TARGET_SPAWN_RATE || maximum != TARGET_MAX_SPAWNS) {
            write_targets();
        }
        usleep(POLL_INTERVAL_US);
    }
    return NULL;
}

static void init_mod(kernel_mod_handle_t *handle) {
    (void)handle;
    if (g_thread_started) return;

    /* Terraria's spawn loop consumes the global fields on Main, not the
       similarly-named fields on NPC. */
    patch_handle_t main_type = patchlib_type_get_type("Terraria", "Main");
    if (!main_type) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ThreeSpawn",
                         "Terraria.Main type not found");
        return;
    }

    g_spawn_rate = patchlib_type_get_field(main_type, "spawnRate");
    g_max_spawns = patchlib_type_get_field(main_type, "maxSpawns");

    if (!g_spawn_rate || !g_max_spawns) {
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ThreeSpawn",
                         "spawnRate or maxSpawns field not found");
        if (g_spawn_rate) patchlib_free(g_spawn_rate);
        if (g_max_spawns) patchlib_free(g_max_spawns);
        g_spawn_rate = PATCH_NULL;
        g_max_spawns = PATCH_NULL;
        patchlib_free(main_type);
        return;
    }

    /* Capture values before any write, so disabling the mod can restore the
       exact parameters that were active at load time. */
    patchlib_field_get_value(g_spawn_rate, PATCH_NULL, &g_base_spawn_rate);
    patchlib_field_get_value(g_max_spawns, PATCH_NULL, &g_base_max_spawns);
    g_base_saved = g_base_spawn_rate > 0 && g_base_max_spawns > 0;

    patchlib_free(main_type);

    /* Apply once synchronously, before the first monitor iteration. */
    write_targets();

    g_running = true;
    if (pthread_create(&g_worker, NULL, monitor_spawn_settings, NULL) != 0) {
        g_running = false;
        mod_logger_write(MOD_LOG_LEVEL_ERROR, "ThreeSpawn",
                         "Monitor thread creation failed");
        return;
    }
    g_thread_started = true;
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ThreeSpawn",
                     "Enabled: Main.spawnRate=200, Main.maxSpawns=15");
}

static void cleanup_mod(kernel_mod_handle_t *handle) {
    (void)handle;
    if (g_thread_started) {
        g_running = false;
        pthread_join(g_worker, NULL);
        g_thread_started = false;
    }

    if (g_base_saved && g_spawn_rate && g_max_spawns) {
        patchlib_field_set_value(g_spawn_rate, PATCH_NULL, &g_base_spawn_rate);
        patchlib_field_set_value(g_max_spawns, PATCH_NULL, &g_base_max_spawns);
    }

    if (g_spawn_rate) patchlib_free(g_spawn_rate);
    if (g_max_spawns) patchlib_free(g_max_spawns);
    g_spawn_rate = PATCH_NULL;
    g_max_spawns = PATCH_NULL;
    g_base_saved = false;
    mod_logger_write(MOD_LOG_LEVEL_INFO, "ThreeSpawn", "Unloaded");
}

static kernel_mod_info_t g_info = {
    .pkg_id = "lzup.gameplay.threespawn",
    .version_code = 2026082601,
    .api_version = 1,
    .version = "1.1.0-test"
};

static kernel_mod_info_t *get_info(void) { return &g_info; }

static kernel_mod_ops_t g_ops = {
    .init_mod = init_mod,
    .cleanup_mod = cleanup_mod,
    .get_info = get_info
};

kernel_mod_ops_t *create_kernel_mod(void) { return &g_ops; }
