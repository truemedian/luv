#!/bin/sh
set -e

lua_engine=$1
lua_mode=$2
libuv_mode=$3
debug_mode=$4

flags="-Ddebug=$debug_mode"

if [ "$lua_mode" = "system" ]; then
    flags="$flags -fsys=lua"

    if [ "$lua_engine" = "lua" ]; then
        # unnecessary for the CI runner, but here for completeness
        flags="$flags -Dlua-pkgconf=lua -Dlua-executable=lua"
    elif [ "$lua_engine" = "luajit" ]; then
        flags="$flags -Dlua-pkgconf=luajit -Dlua-executable=luajit"
    fi
elif [ "$lua_mode" = "luarocks" ]; then
    flags="$flags -fsys=luarocks"
else # bundled
    flags="$flags -Dlua-shared=$lua_mode -Dlua-engine=$lua_engine"
fi

if [ "$libuv_mode" = "system" ]; then
    flags="$flags -fsys=libuv"
else # bundled
    flags="$flags -Dlibuv-shared=$libuv_mode"
fi

echo "Building with $flags"
zig build -p build test $flags
