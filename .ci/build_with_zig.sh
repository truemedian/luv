#!/bin/sh
set -e

lua_engine=$1
lua_mode=$2
libuv_mode=$3
debug_mode=$4

if [ "$debug_mode" = "debug" ]; then
    flags="-Ddebug=true"
elif [ "$debug_mode" = "release" ]; then
    flags="-Ddebug=false"
else
    echo "Invalid debug_mode: $debug_mode"; exit 1
fi

if [ "$lua_mode" = "system" ]; then
    flags="$flags -fsys=lua"

    if [ "$lua_engine" = "Lua" ]; then
        # unnecessary for the CI runner, but here for completeness
        flags="$flags -Dlua-pkgconf=lua -Dlua-executable=lua5.4"
    elif [ "$lua_engine" = "LuaJIT" ]; then
        flags="$flags -Dlua-pkgconf=luajit -Dlua-executable=luajit"
    fi
elif [ "$lua_mode" = "luarocks" ]; then
    flags="$flags -fsys=luarocks"
elif [ "$lua_mode" = "shared" ]; then
    flags="$flags -Dlua-shared=true -Dlua-engine=$lua_engine"
elif [ "$lua_mode" = "static" ]; then
    flags="$flags -Dlua-shared=false -Dlua-engine=$lua_engine"
else
    echo "Invalid lua_mode: $lua_mode"; exit 1
fi

if [ "$libuv_mode" = "system" ]; then
    flags="$flags -fsys=libuv"
elif [ "$libuv_mode" = "shared" ]; then
    flags="$flags -Dlibuv-shared=true"
elif [ "$libuv_mode" = "static" ]; then
    flags="$flags -Dlibuv-shared=false"
else
    echo "Invalid libuv_mode: $lua_mode"; exit 1
fi

echo "Building with $flags"
zig build -p build test $flags
