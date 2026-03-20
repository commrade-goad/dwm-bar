all: main.c
	cc main.c -o dwm-bar -lX11 -O2 -march=native -flto -s -Wall -Wextra
