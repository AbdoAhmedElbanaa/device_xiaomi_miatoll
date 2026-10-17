/*
 * config.c — read persist.vendor.clarifae.* (and persist.clarifae.*) system properties
 * into clarifae_config_t, with cheap change detection for the realtime audio path.
 */
#include "config.h"
#include "log.h"

#include <sys/system_properties.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Single definition of the runtime log level used by log.h everywhere. */
int clarifae_log_level = 1;

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

static int get_str(const char* name_sys, const char* name_vendor, const char* name_legacy,
                   char* out, int outlen, const char* def) {
    char buf[PROP_VALUE_MAX];
    int n = __system_property_get(name_sys, buf);
    if (n <= 0 && name_vendor) {
        n = __system_property_get(name_vendor, buf);
    }
    if (n <= 0 && name_legacy) {
        n = __system_property_get(name_legacy, buf);
    }
    const char* src = (n > 0) ? buf : def;
    strncpy(out, src, (size_t)outlen - 1);
    out[outlen - 1] = '\0';
    return n > 0;
}

static int get_int(const char* name_sys, const char* name_vendor, const char* name_legacy,
                   int def, int lo, int hi) {
    char buf[PROP_VALUE_MAX];
    int n = __system_property_get(name_sys, buf);
    if (n <= 0 && name_vendor) {
        n = __system_property_get(name_vendor, buf);
    }
    if (n <= 0 && name_legacy) {
        n = __system_property_get(name_legacy, buf);
    }
    if (n <= 0) return def;

    char* end = NULL;
    long v = strtol(buf, &end, 10);
    if (end == buf) return def;       /* not a number -> default */
    if (v < lo) v = lo;
    if (v > hi) v = hi;
    return (int)v;
}

void clarifae_config_load(clarifae_config_t* cfg) {
    cfg->enabled      = get_int("persist.vendor.clarifae.enabled",      "persist.clarifae.enabled",      "persist.sys.clarifae.enabled",      1, 0, 1);
    cfg->in_enabled   = get_int("persist.vendor.clarifae.in_enabled",   "persist.clarifae.in_enabled",   "persist.sys.clarifae.in_enabled",   1, 0, 1);
    cfg->out_enabled  = get_int("persist.vendor.clarifae.out_enabled",  "persist.clarifae.out_enabled",  "persist.sys.clarifae.out_enabled",  0, 0, 1);
    cfg->mode         = get_int("persist.vendor.clarifae.mode",         "persist.clarifae.mode",         "persist.sys.clarifae.mode",         1, 1, 3);
    cfg->strength     = get_int("persist.vendor.clarifae.strength",     "persist.clarifae.strength",     "persist.sys.clarifae.strength",     70, 0, 100);
    cfg->out_strength = get_int("persist.vendor.clarifae.out_strength", "persist.clarifae.out_strength", "persist.sys.clarifae.out_strength", 50, 0, 100);
    cfg->atten_db     = get_int("persist.vendor.clarifae.atten_db",     "persist.clarifae.atten_db",     "persist.sys.clarifae.atten_db",     24, 0, 60);
    cfg->vad          = get_int("persist.vendor.clarifae.vad",          "persist.clarifae.vad",          "persist.sys.clarifae.vad",          50, 0, 100);
    cfg->highpass     = get_int("persist.vendor.clarifae.highpass",     "persist.clarifae.highpass",     "persist.sys.clarifae.highpass",     1, 0, 1);
    cfg->autogain     = get_int("persist.vendor.clarifae.autogain",     "persist.clarifae.autogain",     "persist.sys.clarifae.autogain",     0, 0, 1);
    cfg->loglevel     = get_int("persist.vendor.clarifae.loglevel",     "persist.clarifae.loglevel",     "persist.sys.clarifae.loglevel",     1, 0, 3);

    get_str("persist.vendor.clarifae.targets", "persist.clarifae.targets", "persist.sys.clarifae.targets",
            cfg->targets, (int)sizeof(cfg->targets), "mic,voice_comm,camcorder,voice_rec");
    get_str("persist.vendor.clarifae.out_targets", "persist.clarifae.out_targets", "persist.sys.clarifae.out_targets",
            cfg->out_targets, (int)sizeof(cfg->out_targets), "music,voice_call");

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
    static uint32_t last = 0;
    static int initialized = 0;
    uint32_t cur = serial_sum();
    if (!initialized) {
        initialized = 1;
        last = cur;
        return 1; /* force an initial load */
    }
    if (cur != last) {
        last = cur;
        return 1;
    }
    return 0;
}
