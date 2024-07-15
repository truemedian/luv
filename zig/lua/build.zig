pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});

    const static = b.addStaticLibrary(.{
        .name = "lua-static",
        .version = lua_version,

        .target = target,
        .optimize = .ReleaseFast,
        .link_libc = true,
        .strip = true,
        .pic = true,
    });

    const shared = b.addSharedLibrary(.{
        .name = "lua-shared",
        .version = lua_version,

        .target = target,
        .optimize = .ReleaseFast,
        .link_libc = true,
        .strip = true,
        .pic = true,
    });

    const exe = b.addExecutable(.{
        .name = "lua",
        .target = target,
        .optimize = .ReleaseFast,
        .link_libc = true,
        .strip = true,
    });

    var flags = std.ArrayList([]const u8).init(b.allocator);
    static.root_module.linkSystemLibrary("m", .{ .needed = true });
    shared.root_module.linkSystemLibrary("m", .{ .needed = true });

    // Amalgamated list of target flags created using the Lua release tarball's makefile
    if (target.result.os.tag == .windows) {
        flags.append("-DLUA_USE_WINDOWS") catch unreachable;
    } else if (target.result.os.tag == .ios) {
        flags.append("-DLUA_USE_IOS") catch unreachable;
    } else {
        flags.append("-DLUA_USE_POSIX") catch unreachable;
    }

    // This is the list of platforms that the Lua makefile claims to use dlopen
    // The list is not exhaustive, support for other platforms should be added as needed
    if (target.result.os.tag == .linux or
        target.result.os.tag == .aix or
        target.result.os.tag.isBSD() or
        target.result.os.tag.isDarwin() or
        target.result.os.tag.isSolarish())
    {
        flags.append("-DLUA_USE_DLOPEN") catch unreachable;

        static.root_module.linkSystemLibrary("dl", .{ .needed = true });
        shared.root_module.linkSystemLibrary("dl", .{ .needed = true });
    }

    if (target.result.os.tag.isSolarish()) {
        flags.append("-D_REENTRANT") catch unreachable;
    }

    const flags_slice = flags.toOwnedSlice() catch unreachable;
    if (b.lazyDependency("lua", .{})) |lua_dep| {
        const lua_src = lua_dep.path("src");

        static.addCSourceFiles(.{
            .root = lua_src,
            .files = base_sources,
            .flags = flags_slice,
        });

        shared.addCSourceFiles(.{
            .root = lua_src,
            .files = base_sources,
            .flags = flags_slice,
        });

        exe.addCSourceFiles(.{
            .root = lua_src,
            .files = exe_sources,
            .flags = flags_slice,
        });
    }

    exe.linkLibrary(static);

    b.installArtifact(static);
    b.installArtifact(shared);
    b.installArtifact(exe);
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
