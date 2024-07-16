const lua_version = find_ver: {
    const zon = @embedFile("build.zig.zon");

    // Zig 0.13.0 does not provide the .version field to the builder, so parse it ourselves.
    const ver_symbol = std.mem.indexOf(u8, zon, ".version") orelse @panic("could not find zon version");
    const ver_start = std.mem.indexOfScalarPos(u8, zon, ver_symbol, '"') orelse @panic("could not find zon version");
    const ver_end = std.mem.indexOfScalarPos(u8, zon, ver_start + 1, '"') orelse @panic("could not find zon version");

    const ver = std.SemanticVersion.parse(zon[ver_start + 1 .. ver_end]) catch @panic("could not parse zon version");
    break :find_ver ver;
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});

    const target_os = target.result.os;

    const debug = b.option(bool, "debug", "Build with debug information and with apicheck enabled") orelse false;
    const optimize: std.builtin.OptimizeMode = if (debug) .Debug else .ReleaseFast;

    const static = b.addStaticLibrary(.{
        .name = "lua-static",
        .version = lua_version,

        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .strip = !debug,
        .pic = true,
    });

    const shared = b.addSharedLibrary(.{
        .name = "lua-shared",
        .version = lua_version,

        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .strip = !debug,
    });

    const exe = b.addExecutable(.{
        .name = "lua",

        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .strip = !debug,
    });

    b.installArtifact(static);
    b.installArtifact(shared);
    b.installArtifact(exe);

    const lua_dep = b.lazyDependency("lua", .{}) orelse return;

    inline for (.{ static, shared }) |lib| {
        static.root_module.linkSystemLibrary("m", .{ .needed = true });

        if (debug)
            lib.root_module.addCMacro("LUA_USE_APICHECK", "1");

        if (target_os.tag == .windows) {
            lib.root_module.addCMacro("LUA_USE_WINDOWS", "1");
        } else if (target_os.tag == .ios) {
            lib.root_module.addCMacro("LUA_USE_IOS", "1");
        } else {
            lib.root_module.addCMacro("LUA_USE_POSIX", "1");
        }

        // This is the list of platforms that the Lua makefile claims to use dlopen
        // The list is not exhaustive, support for other platforms should be added as needed
        if (target_os.tag == .linux or
            target_os.tag == .aix or
            target_os.tag.isBSD() or
            target_os.tag.isDarwin() or
            target_os.tag.isSolarish())
        {
            lib.root_module.addCMacro("LUA_USE_DLOPEN", "1");

            lib.root_module.linkSystemLibrary("dl", .{ .needed = true });
        }

        if (target_os.tag.isSolarish()) {
            lib.root_module.addCMacro("_REENTRANT", "1");
        }

        lib.root_module.sanitize_c = false;
        lib.addCSourceFiles(.{
            .root = lua_dep.path("src"),
            .files = base_sources,
        });

        lib.installHeader(lua_dep.path("src/lua.h"), "lua.h");
        lib.installHeader(lua_dep.path("src/luaconf.h"), "luaconf.h");
        lib.installHeader(lua_dep.path("src/lualib.h"), "lualib.h");
        lib.installHeader(lua_dep.path("src/lauxlib.h"), "lauxlib.h");
    }

    exe.addCSourceFiles(.{
        .root = lua_dep.path("src"),
        .files = exe_sources,
    });

    exe.linkLibrary(static);
}

const base_sources: []const []const u8 = &.{
    "lapi.c",     "lcode.c",    "lctype.c",   "ldebug.c",  "ldo.c",      "ldump.c",   "lfunc.c",
    "lgc.c",      "llex.c",     "lmem.c",     "lobject.c", "lopcodes.c", "lparser.c", "lstate.c",
    "lstring.c",  "ltable.c",   "ltm.c",      "lundump.c", "lvm.c",      "lzio.c",    "lauxlib.c",
    "lbaselib.c", "lcorolib.c", "ldblib.c",   "liolib.c",  "lmathlib.c", "loadlib.c", "loslib.c",
    "lstrlib.c",  "ltablib.c",  "lutf8lib.c", "linit.c",
};

const exe_sources: []const []const u8 = &.{"lua.c"};

const std = @import("std");
