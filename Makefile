dwm-mode: main.c
	cc main.c -o dwm-bar -lxcb -O2 -march=native -flto -s -Wall -Wextra -DDWM_MODE

stdout-mode: main.c
	cc main.c -o dwm-bar -O2 -march=native -flto -s -Wall -Wextra

.PHONY: dwm-mode stdout-mode
