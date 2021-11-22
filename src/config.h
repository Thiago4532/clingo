#ifndef _CONFIG_H
#define _CONFIG_H

typedef struct {
    char** excluded_windows;
} config_t;

extern config_t config;

int parse_config_file(config_t* cfg, const char* filename);

void free_config(config_t* cfg);

#endif
