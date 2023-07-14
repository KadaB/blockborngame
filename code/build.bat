@echo off

IF NOT EXIST ..\..\build_jam mkdir ..\..\build_jam
pushd ..\..\build_jam

REM C:/emsdk/emsdk activate latest --permanent

emcc -o road.html ../blockborngame/code/main.cpp -Wall -std=c++17 -D_DEFAULT_SOURCE -Wno-missing-braces -Wunused-result -Os -I. -I D:/raylib/raylib/src -I D:/raylib/raylib/src/external -L. -L D:/raylib/raylib/src -s USE_GLFW=3 -s ASYNCIFY -s TOTAL_MEMORY=67108864 -s FORCE_FILESYSTEM=1 --preload-file ../blockborngame@ --shell-file D:/raylib/raylib/src/shell.html D:/raylib/raylib/src/web/libraylib.a -DPLATFORM_WEB -s EXPORTED_FUNCTIONS=["_free","_malloc","_main"] -s EXPORTED_RUNTIME_METHODS=ccall -s ASSERTIONS=1

REM Maybe better sound: -s USE_SDL=2
REM Include before --shell-fil
REM --preload-file Graphics --preload-file Sounds

popd