CFLAGS = -O2 -march=native -flto -s -Wall -Wextra

ifdef TEA
CFLAGS += -DTEA_SUPPORT
endif

dwm-mode: main.c
	cc main.c $(CFLAGS) -DDWM_MODE -lxcb -o dwm-bar

stdout-mode: main.c
	cc main.c -o dwm-bar $(CFLAGS)

.PHONY: dwm-mode stdout-mode
