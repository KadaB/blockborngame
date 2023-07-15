# blockborngame
Hello, world!

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
