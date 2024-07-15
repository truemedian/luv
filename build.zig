pub const LuaEngine = enum { Lua, LuaJIT };

pub fn build(b: *std.Build) void {
    // The optimization level for any binaries.
    const optimize = b.standardOptimizeOption(.{});

    // The compilation target for any binaries.
    const target = b.standardTargetOptions(.{});

    _ = .{ optimize, target };
}

const std = @import("std");

const lua = @import("deps/lua.zig");