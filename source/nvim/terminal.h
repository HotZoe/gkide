/// @file nvim/terminal.h

#ifndef NVIM_TERMINAL_H
#define NVIM_TERMINAL_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct terminal_s terminal_st;
typedef void (*terminal_write_ft)(char *buffer, size_t size, void *data);
typedef void (*terminal_resize_ft)(uint16_t width, uint16_t height, void *data);
typedef void (*terminal_close_ft)(void *data);

typedef struct terminal_opt_s
{
    void *data;
    uint16_t width;
    uint16_t height;
    terminal_write_ft write_cb;
    terminal_resize_ft resize_cb;
    terminal_close_ft close_cb;
} terminal_opt_st;

#ifdef INCLUDE_GENERATED_DECLARATIONS
    #include "terminal.h.generated.h"
#endif

#endif // NVIM_TERMINAL_H
