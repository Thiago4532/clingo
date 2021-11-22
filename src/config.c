#include "config.h"

#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#define BUF_MAX 4096

#ifndef NDEBUG
#define LOG(format, ...)                                              \
    fprintf(stderr, "%s:" STRGY(__LINE__) ": " format "\n", __func__, \
            ##__VA_ARGS__)
#else
#define LOG(...) \
    do {         \
    } while (0)
#endif

#define error(fmt, ...) \
    fprintf(stderr, "%s: " fmt "\n", __func__, ##__VA_ARGS__)

/* Hack to prevent NULL variables */
static void* nullptr = NULL;

config_t config;

static void
config_init(config_t* cfg) {
    cfg->excluded_windows = (char**)&nullptr;
}

static char**
split_words(const char* s) {
    size_t len = 0, cap = 16;
    char** ret = malloc(cap * sizeof *ret);

    const char* ptr = NULL;
    for (; *s; s++) {
        switch (*s) {
        case ' ':
            if (ptr) {
                ret[len++] = strndup(ptr, s - ptr);
                if (len == cap)
                    ret = realloc(ret, (cap *= 2) * sizeof *ret);

                ptr = NULL;
            }

            break;
        default:
            if (!ptr)
                ptr = s;

            break;
        }
    }
    if (ptr) {
        ret[len++] = strdup(ptr);
        if (len == cap)
            ret = realloc(ret, (cap *= 2) * sizeof *ret);
    }

    ret[len] = NULL;
    return ret;
}

int
parse_config_file(config_t* cfg, const char* filename) {
    config_init(cfg);

    FILE* f = fopen(filename, "r");
    if (!f) {
        error("fopen(): %s: %s", filename, strerror(errno));
        return -1;
    }

    int ret = -1;
    char buf[BUF_MAX];
    for (;;) {
        if (!fgets(buf, BUF_MAX, f)) {
            if (!feof(f)) {
                error("fgets: %s", strerror(errno));
                goto cleanup;
            }

            ret = 0;
            goto cleanup;
        }

        size_t size = strlen(buf);
        if (size == 0 || buf[size - 1] != '\n') {
            error("failed to read buf: line length is greater than %d",
                  BUF_MAX);
        }
        buf[--size] = '\0';

        // Parsing key
        char* sep = strchr(buf, ':');
        if (!sep) {
            error("failed to parse line: missing separator");
            goto cleanup;
        }

        char* key = strndup(buf, sep - buf);
        if (strcmp(key, "excluded_windows") == 0) {
            if (cfg->excluded_windows == (char**)&nullptr)
                cfg->excluded_windows = split_words(sep + 1);
            else
                error("duplicated option: %s", key);
        } else {
            error("invalid config option: %s", key);
        }
        free(key);
    }

cleanup:
    fclose(f);
    if (ret != 0)
        free_config(cfg);
    return ret;
}

void
free_config(config_t* cfg) {
    if (cfg->excluded_windows != (char**)&nullptr) {
        for (char** ptr = cfg->excluded_windows; *ptr; ptr++)
            free(*ptr);
        free(cfg->excluded_windows);
    }
}
