/// Adds necessary libraries and flags to a Lua library.
fn configureLibrary(
    lib: *std.Build.Step.Compile,
    flags: *std.ArrayList([]const u8),
    target: std.Build.ResolvedTarget,
) void {
    lib.root_module.linkSystemLibrary("m", .{ .needed = true });

    // Amalgamated list of target flags created using the Lua release tarball's src/Makefile
    if (target.result.os.tag == .windows) {
        flags.append("-DLUA_USE_WINDOWS") catch unreachable;
    } else if (target.result.os.tag == .ios) {
        flags.append("-DLUA_USE_IOS") catch unreachable;
    } else {
        flags.append("-DLUA_USE_POSIX") catch unreachable;
    }

    if (target.result.os.tag == .linux or
        target.result.os.tag == .aix or
        target.result.os.tag.isBSD() or
        target.result.os.tag.isDarwin() or
        target.result.os.tag.isSolarish())
    {
        flags.append("-DLUA_USE_DLOPEN") catch unreachable;

        lib.root_module.linkSystemLibrary("dl", .{ .needed = true });
    }

    if (target.result.os.tag.isSolarish()) {
        flags.append("-D_REENTRANT") catch unreachable;
    }
}

/// Create a static Lua library.
pub fn createStatic(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
) *std.Build.Step.Compile {
    const lib = b.addStaticLibrary(.{
        .name = "lua-static",
        .version = lua_version,

        .target = target,
        .optimize = .ReleaseFast,
        .link_libc = true,
        .strip = true,
        .pic = true,
    });

    var flags = std.ArrayList([]const u8).init(b.allocator);
    configureLibrary(lib, &flags, target);

    if (b.lazyDependency("lua", .{})) |lua_dep|
        lib.addCSourceFiles(.{
            .root = lua_dep.path(""),
            .files = base_sources,
            .flags = flags.toOwnedSlice() catch unreachable,
        });

    return lib;
}

/// Create a dynamic Lua library. Always optimized.
pub fn createShared(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
) *std.Build.Step.Compile {
    const lib = b.addSharedLibrary(.{
        .name = "lua-shared",
        .version = lua_version,

        .target = target,
        .optimize = .ReleaseFast,
        .link_libc = true,
        .strip = true,
        .pic = true,
    });

    var flags = std.ArrayList([]const u8).init(b.allocator);
    configureLibrary(lib, &flags, target);

    if (b.lazyDependency("lua", .{})) |lua_dep|
        lib.addCSourceFiles(.{
            .root = lua_dep.path(""),
            .files = base_sources,
            .flags = flags.toOwnedSlice() catch unreachable,
        });

    return lib;
}

/// Create a Lua executable. Always statically links the Lua library and is always optimized.
pub fn createExecutable(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
) *std.Build.Step.Compile {
    const exe = b.addExecutable(.{
        .name = "lua",
        .target = target,
        .optimize = .ReleaseFast,
        .link_libc = true,
        .strip = true,
    });

    var flags = std.ArrayList([]const u8).init(b.allocator);
    configureLibrary(exe, &flags, target);

    if (b.lazyDependency("lua", .{})) |lua_dep|
        exe.addCSourceFiles(.{
            .root = lua_dep.path(""),
            .files = exe_sources,
            .flags = flags.toOwnedSlice() catch unreachable,
        });

    const static = createStatic(b, target);
    exe.root_module.linkLibrary(static);

    return exe;
}

const lua_version = std.SemanticVersion{ .major = 5, .minor = 4, .patch = 7 };

const base_sources: []const []const u8 = &.{
    "lapi.c",     "lcode.c",    "lctype.c",   "ldebug.c",  "ldo.c",      "ldump.c",   "lfunc.c",
    "lgc.c",      "llex.c",     "lmem.c",     "lobject.c", "lopcodes.c", "lparser.c", "lstate.c",
    "lstring.c",  "ltable.c",   "ltm.c",      "lundump.c", "lvm.c",      "lzio.c",    "lauxlib.c",
    "lbaselib.c", "lcorolib.c", "ldblib.c",   "liolib.c",  "lmathlib.c", "loadlib.c", "loslib.c",
    "lstrlib.c",  "ltablib.c",  "lutf8lib.c", "linit.c",
};

const exe_sources: []const []const u8 = &.{"lua.c"};

const std = @import("std");
