
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


#include "datoviz/_log.h"
#include <emscripten/emscripten.h>



EM_JS(void, js_console_log, (const char *msg), {
    console.log(UTF8ToString(msg));
});


EM_JS(void, js_console_warn, (const char *msg), {
    console.warn(UTF8ToString(msg));
});


EM_JS(void, js_console_error, (const char *msg), {
    console.error(UTF8ToString(msg));
});


void log_log(int level, const char *file, int line, const char *fmt, ...) {
    char msg[1024];
    char full[1200];

    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    snprintf(full, sizeof(full), "[%s:%d] %s", file, line, msg);

    if (level >= LOG_ERROR)
        js_console_error(full);
    else if (level >= LOG_WARN)
        js_console_warn(full);
    else
        js_console_log(full);
}
