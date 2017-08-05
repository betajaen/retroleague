emcc main.c synthwave.c -g -O3 -s WASM=1 -s USE_SDL=2 -s USE_SDL_NET=2 --embed-file assets --shell-file emscriptem_shell.html -o bin/game.html
