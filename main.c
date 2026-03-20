#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <X11/Xlib.h>

#define SLEEP_TIME 30
#define BAT_PATH "/sys/class/power_supply/BAT0/"
// #define DATE_FORMAT "%d-%02d-%02d %02d:%02d:%02d"
#define DATE_FORMAT "%02d:%02d - %02d/%02d/%d"
#define DATE_INSIDE(tm) (tm).tm_hour, (tm).tm_min, (tm).tm_mday, (tm).tm_mon + 1, (tm).tm_year + 1900

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

    // Open the x11 display
    Display *d = XOpenDisplay(NULL);
    if (!d) return 1;
    const size_t len = 16;

    char percentage[len] = {};
    char status[len] = {};
    char buffer[256] = {};

    struct timespec req = {5, 0};

    while (running) {
        // Read the battery file
        FILE *fcap = fopen(BAT_PATH"capacity", "r");
        FILE *fsta = fopen(BAT_PATH"status", "r");
        if (!fcap) return 1;
        if (!fsta) return 1;

        // Get the current time
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);

        // Fill with zeros
        memset(status, 0, sizeof(status));
        memset(percentage, 0, sizeof(percentage));

        // == Battery stuff
        size_t n = fread(percentage, sizeof(char), len - 1, fcap);
        percentage[n] = '\0';
        assert(strlen(percentage) >= 2);
        percentage[strlen(percentage) - 1] = 0;

        n = fread(status, sizeof(char), len - 1, fsta);
        status[n] = '\0';
        assert(strlen(status) >= 2);
        status[strlen(status) - 1] = 0;

        bool is_charging = strcmp(status, "Charging") == 0;

        // == Buffer output stuff
        snprintf(buffer, sizeof(buffer), "[B: %s%%%c] [D: "DATE_FORMAT"]",
                 percentage,
                 is_charging ? '+' : '~',
                 DATE_INSIDE(tm)
        );

        // == X11 stuff
        XStoreName(d, DefaultRootWindow(d), buffer);
        XSync(d, False);

        fclose(fcap);
        fclose(fsta);

        nanosleep(&req, NULL);
    }
    XCloseDisplay(d);
    return 0;
}
