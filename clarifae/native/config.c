/*
 * config.c — read clarifae configuration from config.conf file and system properties
 * into clarifae_config_t, with cheap change detection for the realtime audio path.
 */
#include "config.h"
#include "log.h"

#include <sys/system_properties.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Single definition of the runtime log level used by log.h everywhere. */
int clarifae_log_level = 1;

static const char* const kConfigPaths[] = {
    "/data/misc/clarifae/config.conf",
    "/data/vendor/clarifae/config.conf",
    "/data/adb/clarifae/config.conf",
    "/vendor/etc/clarifae/config.conf",
    "/vendor/etc/clarifae.conf",
};
#define NCONFIG_PATHS (sizeof(kConfigPaths) / sizeof(kConfigPaths[0]))

static const char* const kProps[] = {
    "persist.sys.clarifae.enabled",
    "persist.sys.clarifae.in_enabled",
    "persist.sys.clarifae.out_enabled",
    "persist.sys.clarifae.mode",
    "persist.sys.clarifae.strength",
    "persist.sys.clarifae.out_strength",
    "persist.sys.clarifae.atten_db",
    "persist.sys.clarifae.vad",
    "persist.sys.clarifae.highpass",
    "persist.sys.clarifae.autogain",
    "persist.sys.clarifae.loglevel",
    "persist.sys.clarifae.targets",
    "persist.sys.clarifae.out_targets",
    "persist.vendor.clarifae.enabled",
    "persist.vendor.clarifae.in_enabled",
    "persist.vendor.clarifae.out_enabled",
    "persist.vendor.clarifae.mode",
    "persist.vendor.clarifae.strength",
    "persist.vendor.clarifae.out_strength",
    "persist.vendor.clarifae.atten_db",
    "persist.vendor.clarifae.vad",
    "persist.vendor.clarifae.highpass",
    "persist.vendor.clarifae.autogain",
    "persist.vendor.clarifae.loglevel",
    "persist.vendor.clarifae.targets",
    "persist.vendor.clarifae.out_targets",
    "persist.clarifae.enabled",
    "persist.clarifae.in_enabled",
    "persist.clarifae.out_enabled",
    "persist.clarifae.mode",
    "persist.clarifae.strength",
    "persist.clarifae.out_strength",
    "persist.clarifae.atten_db",
    "persist.clarifae.vad",
    "persist.clarifae.highpass",
    "persist.clarifae.autogain",
    "persist.clarifae.loglevel",
    "persist.clarifae.targets",
    "persist.clarifae.out_targets",
};
#define NPROPS (sizeof(kProps) / sizeof(kProps[0]))

static const char* find_config_file(void) {
    for (size_t i = 0; i < NCONFIG_PATHS; i++) {
        struct stat st;
        if (stat(kConfigPaths[i], &st) == 0 && S_ISREG(st.st_mode)) {
            return kConfigPaths[i];
        }
    }
    return NULL;
}

static const char* const kDefaultConfigContent =
    "# Clarifae Configuration File\n"
    "enabled=1\n"
    "in_enabled=1\n"
    "out_enabled=0\n"
    "mode=1\n"
    "strength=70\n"
    "out_strength=60\n"
    "atten_db=24\n"
    "vad=50\n"
    "targets=mic,voice_comm,camcorder,voice_rec\n"
    "out_targets=music,voice_call,system\n"
    "highpass=1\n"
    "autogain=0\n"
    "loglevel=1\n";

static const char* ensure_config_file(void) {
    const char* conf_path = find_config_file();
    if (conf_path) return conf_path;

    /* If no config file exists anywhere, create one with default settings */
    const char* candidates[] = {
        "/data/misc/clarifae/config.conf",
        "/data/vendor/clarifae/config.conf",
        "/data/adb/clarifae/config.conf",
    };

    for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        const char* path = candidates[i];
        char dir[256];
        strncpy(dir, path, sizeof(dir) - 1);
        dir[sizeof(dir) - 1] = '\0';
        char* last_slash = strrchr(dir, '/');
        if (last_slash) {
            *last_slash = '\0';
            mkdir(dir, 0775);
        }

        FILE* fp = fopen(path, "w");
        if (fp) {
            fputs(kDefaultConfigContent, fp);
            fclose(fp);
            chmod(path, 0664);
            CLARIFAE_LOGI("Created new default config.conf at %s", path);
            return path;
        }
    }
    return NULL;
}

static char* trim(char* s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char* end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

static void parse_config_file(const char* path, clarifae_config_t* cfg) {
    FILE* fp = fopen(path, "r");
    if (!fp) return;

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char* p = trim(line);
        if (*p == '\0' || *p == '#') continue;

        char* eq = strchr(p, '=');
        if (!eq) continue;

        *eq = '\0';
        char* k = trim(p);
        char* v = trim(eq + 1);

        if (strcmp(k, "enabled") == 0) {
            cfg->enabled = (atoi(v) != 0) ? 1 : 0;
        } else if (strcmp(k, "in_enabled") == 0) {
            cfg->in_enabled = (atoi(v) != 0) ? 1 : 0;
        } else if (strcmp(k, "out_enabled") == 0) {
            cfg->out_enabled = (atoi(v) != 0) ? 1 : 0;
        } else if (strcmp(k, "mode") == 0) {
            int m = atoi(v);
            if (m >= 1 && m <= 3) cfg->mode = m;
        } else if (strcmp(k, "strength") == 0) {
            int val = atoi(v);
            if (val < 0) val = 0; else if (val > 100) val = 100;
            cfg->strength = val;
        } else if (strcmp(k, "out_strength") == 0) {
            int val = atoi(v);
            if (val < 0) val = 0; else if (val > 100) val = 100;
            cfg->out_strength = val;
        } else if (strcmp(k, "atten_db") == 0) {
            int val = atoi(v);
            if (val < 0) val = 0; else if (val > 60) val = 60;
            cfg->atten_db = val;
        } else if (strcmp(k, "vad") == 0) {
            int val = atoi(v);
            if (val < 0) val = 0; else if (val > 100) val = 100;
            cfg->vad = val;
        } else if (strcmp(k, "highpass") == 0) {
            cfg->highpass = (atoi(v) != 0) ? 1 : 0;
        } else if (strcmp(k, "autogain") == 0) {
            cfg->autogain = (atoi(v) != 0) ? 1 : 0;
        } else if (strcmp(k, "loglevel") == 0) {
            int val = atoi(v);
            if (val < 0) val = 0; else if (val > 3) val = 3;
            cfg->loglevel = val;
        } else if (strcmp(k, "targets") == 0) {
            strncpy(cfg->targets, v, sizeof(cfg->targets) - 1);
            cfg->targets[sizeof(cfg->targets) - 1] = '\0';
        } else if (strcmp(k, "out_targets") == 0) {
            strncpy(cfg->out_targets, v, sizeof(cfg->out_targets) - 1);
            cfg->out_targets[sizeof(cfg->out_targets) - 1] = '\0';
        }
    }
    fclose(fp);
    CLARIFAE_LOGI("loaded config from file %s: enabled=%d in=%d out=%d mode=%d str=%d",
                  path, cfg->enabled, cfg->in_enabled, cfg->out_enabled, cfg->mode, cfg->strength);
}

static int get_str(const char* name_sys, const char* name_vendor, const char* name_legacy,
                   char* out, int outlen) {
    char buf[PROP_VALUE_MAX];
    int n = __system_property_get(name_sys, buf);
    if (n <= 0 && name_vendor) {
        n = __system_property_get(name_vendor, buf);
    }
    if (n <= 0 && name_legacy) {
        n = __system_property_get(name_legacy, buf);
    }
    if (n > 0 && buf[0] != '\0') {
        strncpy(out, buf, (size_t)outlen - 1);
        out[outlen - 1] = '\0';
        return 1;
    }
    return 0;
}

static int get_int(const char* name_sys, const char* name_vendor, const char* name_legacy,
                   int* out, int lo, int hi) {
    char buf[PROP_VALUE_MAX];
    int n = __system_property_get(name_sys, buf);
    if (n <= 0 && name_vendor) {
        n = __system_property_get(name_vendor, buf);
    }
    if (n <= 0 && name_legacy) {
        n = __system_property_get(name_legacy, buf);
    }
    if (n <= 0 || buf[0] == '\0') return 0;

    char* end = NULL;
    long v = strtol(buf, &end, 10);
    if (end == buf) return 0;
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    *out = (int)v;
    return 1;
}

void clarifae_config_load(clarifae_config_t* cfg) {
    /* 1. Set default base values */
    cfg->enabled      = 1;
    cfg->in_enabled   = 1;
    cfg->out_enabled  = 0;
    cfg->mode         = 1;
    cfg->strength     = 70;
    cfg->out_strength = 60;
    cfg->atten_db     = 24;
    cfg->vad          = 50;
    cfg->highpass     = 1;
    cfg->autogain     = 0;
    cfg->loglevel     = 1;
    strncpy(cfg->targets, "mic,voice_comm,camcorder,voice_rec", sizeof(cfg->targets) - 1);
    cfg->targets[sizeof(cfg->targets) - 1] = '\0';
    strncpy(cfg->out_targets, "music,voice_call", sizeof(cfg->out_targets) - 1);
    cfg->out_targets[sizeof(cfg->out_targets) - 1] = '\0';

    /* 2. Load from config.conf if present, or create default config.conf */
    const char* conf_path = ensure_config_file();
    if (conf_path) {
        parse_config_file(conf_path, cfg);
    }

    /* 3. System properties can override if explicitly set */
    get_int("persist.sys.clarifae.enabled",      "persist.vendor.clarifae.enabled",      "persist.clarifae.enabled",      &cfg->enabled,      0, 1);
    get_int("persist.sys.clarifae.in_enabled",   "persist.vendor.clarifae.in_enabled",   "persist.clarifae.in_enabled",   &cfg->in_enabled,   0, 1);
    get_int("persist.sys.clarifae.out_enabled",  "persist.vendor.clarifae.out_enabled",  "persist.clarifae.out_enabled",  &cfg->out_enabled,  0, 1);
    get_int("persist.sys.clarifae.mode",         "persist.vendor.clarifae.mode",         "persist.clarifae.mode",         &cfg->mode,         1, 3);
    get_int("persist.sys.clarifae.strength",     "persist.vendor.clarifae.strength",     "persist.clarifae.strength",     &cfg->strength,     0, 100);
    get_int("persist.sys.clarifae.out_strength", "persist.vendor.clarifae.out_strength", "persist.clarifae.out_strength", &cfg->out_strength, 0, 100);
    get_int("persist.sys.clarifae.atten_db",     "persist.vendor.clarifae.atten_db",     "persist.clarifae.atten_db",     &cfg->atten_db,     0, 60);
    get_int("persist.sys.clarifae.vad",          "persist.vendor.clarifae.vad",          "persist.clarifae.vad",          &cfg->vad,          0, 100);
    get_int("persist.sys.clarifae.highpass",     "persist.vendor.clarifae.highpass",     "persist.clarifae.highpass",     &cfg->highpass,     0, 1);
    get_int("persist.sys.clarifae.autogain",     "persist.vendor.clarifae.autogain",     "persist.clarifae.autogain",     &cfg->autogain,     0, 1);
    get_int("persist.sys.clarifae.loglevel",     "persist.vendor.clarifae.loglevel",     "persist.clarifae.loglevel",     &cfg->loglevel,     0, 3);

    get_str("persist.sys.clarifae.targets", "persist.vendor.clarifae.targets", "persist.clarifae.targets",
            cfg->targets, (int)sizeof(cfg->targets));
    get_str("persist.sys.clarifae.out_targets", "persist.vendor.clarifae.out_targets", "persist.clarifae.out_targets",
            cfg->out_targets, (int)sizeof(cfg->out_targets));

    clarifae_log_level = cfg->loglevel;
}

/* Sum of per-property serials; changes whenever any clarifae prop is written. */
static uint32_t serial_sum(void) {
    uint32_t s = 0;
    for (size_t i = 0; i < NPROPS; i++) {
        const prop_info* pi = __system_property_find(kProps[i]);
        if (pi) s += __system_property_serial(pi);
    }
    return s;
}

int clarifae_config_changed(void) {
    static uint32_t last_serial = 0;
    static time_t last_mtime = 0;
    static off_t last_size = 0;
    static int initialized = 0;

    uint32_t cur_serial = serial_sum();
    time_t cur_mtime = 0;
    off_t cur_size = 0;

    const char* conf_path = find_config_file();
    if (conf_path) {
        struct stat st;
        if (stat(conf_path, &st) == 0) {
            cur_mtime = st.st_mtime;
            cur_size = st.st_size;
        }
    }

    if (!initialized) {
        initialized = 1;
        last_serial = cur_serial;
        last_mtime = cur_mtime;
        last_size = cur_size;
        return 1; /* force an initial load */
    }

    if (cur_serial != last_serial || cur_mtime != last_mtime || cur_size != last_size) {
        last_serial = cur_serial;
        last_mtime = cur_mtime;
        last_size = cur_size;
        return 1;
    }
    return 0;
}
