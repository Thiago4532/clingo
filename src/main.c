#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glib-unix.h>
#include <gtk/gtk.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#define STRINGFY(x) #x
#define STRGY(x) STRINGFY(x)

#ifndef NDEBUG
#define LOG(format, ...) \
    g_printerr("%s:" STRGY(__LINE__) ": " format "\n", __func__, ##__VA_ARGS__)
#else
#define LOG(...) \
    do {         \
    } while (0)
#endif

static GtkClipboard* clipboard;
static gchar* clipboard_text;

static Atom a_Clipboard = None;
static Atom a_netWmClass = None;
static Display* display = NULL;

static gboolean
is_selection_from_excluded_window(Atom selection_type) {
    gboolean excluded = FALSE;
    XTextProperty text_prop_return;
    gchar* window_class = NULL;

    Window window = XGetSelectionOwner(display, selection_type);
    if (window != None)
        if (XGetTextProperty(display, window, &text_prop_return, a_netWmClass))
            window_class = (gchar*)text_prop_return.value;

    if (!window_class) {
        int tmp;
        XGetInputFocus(display, &window, &tmp);

        if (XGetTextProperty(display, window, &text_prop_return, a_netWmClass))
            window_class = (gchar*)text_prop_return.value;
    }

    if (window_class) {
        LOG("window class: '%s'", window_class);

        GRegex* regexp = NULL;
        for (char** ex_window = config.excluded_windows; *ex_window;
             ex_window++) {
            if (strcmp(*ex_window, window_class) == 0) {
                excluded = TRUE;
                break;
            }
        }
    } else
        LOG("failed to get window class!");

    if (excluded)
        LOG("excluded window!");
    return excluded;
}

static gboolean
clipboard_handler(gpointer data) {
    gchar* clipboard_temp = gtk_clipboard_wait_for_text(clipboard);

    /* Check if clipboard contents were lost */
    if (!clipboard_temp && clipboard_text) {
        gint count;
        GdkAtom* targets;
        gboolean contents =
            gtk_clipboard_wait_for_targets(clipboard, &targets, &count);
        g_free(targets);

        if (!contents) {
            g_print("Clipboard is null, recovering ...\n");
            gtk_clipboard_set_text(clipboard, clipboard_text, -1);
            goto cleanup;
        } else {
            clipboard_temp = g_strdup("");
        }
    }
    if (g_strcmp0(clipboard_temp, clipboard_text) != 0 &&
        !is_selection_from_excluded_window(a_Clipboard)) {
        LOG("clipboard was modified!");
        g_free(clipboard_text);
        clipboard_text = g_strdup(clipboard_temp);
    }

cleanup:
    g_free(clipboard_temp);
    return TRUE;
}

static volatile gboolean signal_received = False;

static gboolean
exit_service(gpointer data) {
    LOG("signal received!");
    if (!signal_received) {
        signal_received = True;
        gtk_main_quit();
    }
    return TRUE;
}

static const char*
gethome() {
    const char* homedir = getenv("HOME");
    if (!homedir)
        homedir = getpwuid(getuid())->pw_dir;

    return homedir;
}

int
main(int argc, char* argv[]) {
    char* config_file = getenv("XDG_CONFIG_HOME");
    if (config_file)
        config_file = g_strdup_printf("%s/clingo", config_file);
    else
        config_file = g_strdup_printf("%s/.config/clingo", gethome());

    LOG("config file: %s", config_file);

    if (parse_config_file(&config, config_file) != 0) {
        g_free(config_file);
        return 1;
    }
    g_free(config_file);

#ifndef NDEBUG
    fprintf(stderr, "[config]: excluded_windows: [");
    for (char** ptr = config.excluded_windows; *ptr; ptr++) {
        if (ptr != config.excluded_windows)
            fprintf(stderr, ", %s", *ptr);
        else
            fprintf(stderr, "%s", *ptr);
    }
    fprintf(stderr, "]\n");
#endif

    gtk_init(&argc, &argv);
    g_unix_signal_add(SIGTERM, exit_service, NULL);
    g_unix_signal_add(SIGINT, exit_service, NULL);
    g_unix_signal_add(SIGHUP, exit_service, NULL);

    clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    g_signal_connect(clipboard, "owner-change", G_CALLBACK(clipboard_handler),
                     NULL);

    display = XOpenDisplay(NULL);
    a_Clipboard = XInternAtom(display, "CLIPBOARD", True);
    a_netWmClass = XInternAtom(display, "WM_CLASS", True);

    gtk_main();
    LOG("freeing resources...");

    free_config(&config);
    g_free(clipboard_text);
    XCloseDisplay(display);
    return 0;
}
