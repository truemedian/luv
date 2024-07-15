pub const LuaEngine = enum { Lua, LuaJIT };

pub fn build(b: *std.Build) void {
    // The optimization level for any binaries.
    const optimize = b.standardOptimizeOption(.{});

    // The compilation target for any binaries.
    const target = b.standardTargetOptions(.{});

    _ = .{ optimize, target };

    const lua = b.dependency("lua", .{ .target = target });

    b.installArtifact(lua.artifact("lua-static"));
    b.installArtifact(lua.artifact("lua-shared"));
    b.installArtifact(lua.artifact("lua"));
}

const std = @import("std");
