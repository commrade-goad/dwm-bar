#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <time.h>

#ifdef DWM_MODE
#include <xcb/xcb.h>
#endif

#define SLEEP_TIME 5
#define BAT_PATH "/sys/class/power_supply/BAT0/"
#define DATE_FORMAT "%02d:%02d - %02d/%02d/%d"
#define DATE_INSIDE(tm) (tm).tm_hour, (tm).tm_min, (tm).tm_mday, (tm).tm_mon + 1, (tm).tm_year + 1900

#ifdef TEA_SUPPORT

#undef SLEEP_TIME
#define SLEEP_TIME 1
#define TEA_PATH "/tmp/idle_inhibit"
static inline bool inhibited() { return access(TEA_PATH, F_OK) == 0; }

#endif

static volatile sig_atomic_t running = 1;

void handle_sigterm(int sig) {
    (void)sig;
    running = 0;
}

int main(void) {
    // Handle sigterm
    struct sigaction sa = {0};
    sa.sa_handler = handle_sigterm;
    sigaction(SIGTERM, &sa, NULL);


#ifdef DWM_MODE
    // Connect to X server
    int screen_nbr;
    xcb_connection_t *conn = xcb_connect(NULL, &screen_nbr);
    if (xcb_connection_has_error(conn)) return 1;

    const xcb_setup_t *setup = xcb_get_setup(conn);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);

    for (int i = 0; i < screen_nbr; ++i)
        xcb_screen_next(&iter);

    xcb_screen_t *screen = iter.data;
#endif

    const size_t len = 16;

    char percentage[len] = {};
    char status[len] = {};
    char buffer[256] = {};

    struct timespec req = {SLEEP_TIME, 0};

    while (running) {
        // Read the battery file
        FILE *fcap = fopen(BAT_PATH "capacity", "r");
        FILE *fsta = fopen(BAT_PATH "status", "r");
        if (!fcap) return 1;
        if (!fsta) return 1;

        // Get the current time
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);

        // == Battery stuff
        size_t n = fread(percentage, 1, len - 1, fcap);
        percentage[n] = '\0';
        assert(strlen(percentage) >= 2);
        percentage[strlen(percentage) - 1] = 0;

        n = fread(status, 1, len - 1, fsta);
        status[n] = '\0';
        assert(strlen(status) >= 2);
        status[strlen(status) - 1] = 0;

        bool is_charging = strcmp(status, "Charging") == 0;

        #ifdef TEA_SUPPORT
        const char *tea_icon = "[ c[_] pkill -9 sleep ]";
        #endif

        // == Buffer output stuff

#ifdef TEA_SUPPORT
        snprintf(buffer, sizeof(buffer),
                 "%s [B: %c%s%%] [D: " DATE_FORMAT "]",
                 inhibited() ? tea_icon : "",
                 is_charging ? '+' : ' ',
                 percentage,
                 DATE_INSIDE(tm));
#else
        snprintf(buffer, sizeof(buffer),
                 "[B: %c%s%%] [D: " DATE_FORMAT "]",
                 is_charging ? '+' : ' ',
                 percentage,
                 DATE_INSIDE(tm));
#endif


#ifdef DWM_MODE
        // == XCB stuff
        xcb_change_property(
            conn,
            XCB_PROP_MODE_REPLACE,
            screen->root,
            XCB_ATOM_WM_NAME,
            XCB_ATOM_STRING,
            8,
            strlen(buffer),
            buffer
        );

        xcb_flush(conn);
#else
        fprintf(stdout, "%s\n", buffer);
	fflush(stdout);
#endif

        fclose(fcap);
        fclose(fsta);

        nanosleep(&req, NULL);
    }


#ifdef DWM_MODE
    xcb_disconnect(conn);
#endif
    return 0;
}
