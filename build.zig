pub const LuaEngine = enum { Lua, LuaJIT };

pub fn build(b: *std.Build) void {
    // The optimization level for any binaries.
    const optimize = b.standardOptimizeOption(.{});

    // The compilation target for any binaries.
    const target = b.standardTargetOptions(.{});

    _ = .{ optimize, target };

    const lua = b.dependency("libuv", .{ .target = target });

    b.installArtifact(lua.artifact("libuv-static"));
    b.installArtifact(lua.artifact("libuv-shared"));

    b.installArtifact(lua.artifact("test-libuv-static"));
    b.installArtifact(lua.artifact("test-libuv-shared"));
    // b.installArtifact(lua.artifact("lua"));
}

const std = @import("std");
