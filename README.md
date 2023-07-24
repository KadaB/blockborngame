# blockborngame
A retro racer game, where you have to fight an alien cruiser and dodge it's attacks. While trying to shoot at it with a lazer.

The lazer only shoots when aiming at the alien.

WASD or arrow keys for driving:
Up/W: Accelerat
Down/S: Break
Left/A: Turn left
Right/D: Turn right

Left click: Shoot lazer gun.

Have fun.

# compile desktop

    mkdir build
    cd build
    cmake ..
    make

# compile web

    mkdir build_web && cd build_web
    emcmake cmake -DPLATFORM=Web ..
    make

start server with

    python3 -m http.server

# Credits/Assets
Programming: Moritz Falk (twitter: @falk_moritz, ig: @falk_dev)
Programming: KadaB (https://github.com/KadaB)
Artist: Weena Lee (ig: @mer_weenana)

Framework: Raylib
City Skyline: https://free-game-assets.itch.io/free-city-backgrounds-pixel-art
Audio files: https://mixkit.co/free-sound-effects/sci-fi/​​
