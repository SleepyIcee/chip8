

os="linux"

if [ $os == "linux" ]; then
    gcc -g -o chip8 src/main.c src/chip8.c src/display.c -Iinclude lib/libraylib.a -lGL -lm -lpthread -ldl -lrt -lX11
elif [ $os == "windows" ]; then
    gcc -g -o chip8.exe src/main.c src/chip8.c src/display.c -Iinclude -Llib -lraylib -lgdi32 -lwinmme
fi