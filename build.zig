const luv_version = find_ver: {
    const zon = @embedFile("build.zig.zon");

    // Zig 0.13.0 does not provide the .version field to the builder, so parse it ourselves.
    const ver_symbol = std.mem.indexOf(u8, zon, ".version") orelse @panic("could not find zon version");
    const ver_start = std.mem.indexOfScalarPos(u8, zon, ver_symbol, '"') orelse @panic("could not find zon version");
    const ver_end = std.mem.indexOfScalarPos(u8, zon, ver_start + 1, '"') orelse @panic("could not find zon version");

    const ver = std.SemanticVersion.parse(zon[ver_start + 1 .. ver_end]) catch @panic("could not parse zon version");
    break :find_ver ver;
};

pub fn build(b: *std.Build) void {
    // The optimization level for any binaries.
    const optimize = b.standardOptimizeOption(.{});

    // The compilation target for any binaries.
    const target = b.standardTargetOptions(.{});

    const debug = b.option(bool, "debug", "Build with debug information and with Lua apicheck enabled") orelse false;
    const libuv_shared = b.option(bool, "libuv-shared", "Build a shared libuv") orelse false;

    const lua_engine = b.option(LuaEngine, "lua-engine", "The Lua implementation to use") orelse .LuaJIT;
    const lua_shared = b.option(bool, "lua-shared", "Build a shared Lua library") orelse false;

    // Integration options
    const want_libuv_integration = b.systemIntegrationOption("libuv", .{});
    const want_luarocks_integration = b.systemIntegrationOption("luarocks", .{ .default = false });
    const want_lua_integration = b.systemIntegrationOption("lua", .{});

    const libuv_pkgconf_name = b.option(
        []const u8,
        "libuv-pkgconf",
        "The name of the libuv library for pkg-config. Only used for system libuv integration.",
    );

    const libuv_libraries = b.option(
        []const u8,
        "libuv-libraries",
        "A comma-separated list of libraries to link against when using the system libuv, including libuv itself. Only used for system libuv integration.",
    ) orelse "";

    const libuv_includes = b.option(
        []const u8,
        "libuv-includes",
        "A comma-separated list of directories to include when using the system libuv, including libuv itself. Only used for system libuv integration.",
    ) orelse "";

    const luarocks_name = b.option(
        []const u8,
        "luarocks",
        "The name or path of the luarocks executable. Only used for system luarocks integration.",
    ) orelse "luarocks";

    const lua_pkgconf_name = b.option(
        []const u8,
        "lua-pkgconf",
        "The name of the lua library for pkg-config. Only used for system lua integration.",
    );

    const lua_libraries = b.option(
        []const u8,
        "lua-libraries",
        "A comma-separated list of libraries to link against when using the system lua, including lua itself. Only used for system lua integration.",
    ) orelse "";

    const lua_includes = b.option(
        []const u8,
        "lua-includes",
        "A comma-separated list of directories to include when using the system lua, including lua itself. Only used for system lua integration.",
    ) orelse "";

    const lua_exe = b.option(
        []const u8,
        "lua-executable",
        "The path to the system Lua executable. Only used for system lua integration.",
    ) orelse "lua";

    const static = b.addStaticLibrary(.{
        .name = "luv-static",
        .version = luv_version,

        .single_threaded = false,
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .strip = !debug,
        .pic = true,

        .unwind_tables = true,
    });

    const shared = b.addSharedLibrary(.{
        .name = "luv-shared",
        .version = luv_version,

        .single_threaded = false,
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .strip = !debug,

        .unwind_tables = true,
    });

    const module = b.addSharedLibrary(.{
        .name = "luv",

        .single_threaded = false,
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .strip = !debug,

        .unwind_tables = true,
    });

    const install_static = b.addInstallArtifact(static, .{});
    const install_shared = b.addInstallArtifact(shared, .{});
    const install_module = b.addInstallArtifact(module, .{
        .dest_sub_path = b.fmt("luv{s}", .{target.result.dynamicLibSuffix()}),
    });

    const static_step = b.step("static", "Build static library");
    static_step.dependOn(&install_static.step);

    const shared_step = b.step("shared", "Build shared library");
    shared_step.dependOn(&install_shared.step);

    const module_step = b.step("module", "Build Lua module");
    module_step.dependOn(&install_module.step);

    b.getInstallStep().dependOn(static_step);
    b.getInstallStep().dependOn(shared_step);
    b.getInstallStep().dependOn(module_step);

    const lua: *std.Build.Dependency = if (!want_luarocks_integration and !want_lua_integration) switch (lua_engine) {
        .Lua => b.dependency("lua", .{ .target = target, .debug = debug }),
        .LuaJIT => b.dependency("luajit", .{ .target = target, .debug = debug }),
    } else undefined;

    var luarocks_config: struct {
        link_lua_explicitly: bool,
        variables: struct {
            LUA_INCDIR: []const u8,
            LUA_LIBDIR: []const u8,
            LUA_LIBDIR_FILE: ?[]const u8 = null,
            LUA: []const u8,
        },
    } = undefined;

    if (want_luarocks_integration) {
        const luarocks_config_str = b.run(&.{ luarocks_name, "config", "--json" });

        luarocks_config = std.json.parseFromSliceLeaky(
            @TypeOf(luarocks_config),
            b.allocator,
            luarocks_config_str,
            .{ .ignore_unknown_fields = true },
        ) catch @panic("could not parse luarocks config");
    }

    inline for (.{ static, shared, module }) |lib| {
        if (want_libuv_integration) system_libuv: {
            // rely on pkg-config when no libraries or includes are provided or when a pkg-config name is provided
            if (libuv_pkgconf_name != null or (libuv_libraries.len == 0 and libuv_includes.len == 0)) {
                lib.root_module.linkSystemLibrary(libuv_pkgconf_name orelse "libuv", .{ .needed = true });

                break :system_libuv;
            }

            // use the provided libraries and includes
            var includes_it = std.mem.splitScalar(u8, libuv_includes, ',');
            while (includes_it.next()) |include_str|
                lib.root_module.addSystemIncludePath(.{ .cwd_relative = include_str });

            var libs_it = std.mem.splitScalar(u8, libuv_libraries, ',');
            while (libs_it.next()) |lib_str|
                lib.root_module.linkSystemLibrary(lib_str, .{ .needed = true });
        } else {
            const libuv = b.dependency("libuv", .{ .target = target, .debug = debug });

            if (libuv_shared) {
                lib.root_module.linkLibrary(libuv.artifact("libuv-shared"));
            } else {
                lib.root_module.linkLibrary(libuv.artifact("libuv-static"));
            }

            // expose libuv artifacts to dependents
            b.installArtifact(libuv.artifact("libuv-shared"));
            b.installArtifact(libuv.artifact("libuv-static"));
        }

        // Assume that the shared libuv is used when the libuv integration is enabled on Windows.
        if ((want_libuv_integration or libuv_shared) and target.result.os.tag == .windows) {
            lib.root_module.addCMacro("USING_UV_SHARED", "1");
        }

        if (want_lua_integration) system_lua: {
            // rely on pkg-config when no libraries or includes are provided or when a pkg-config name is provided
            if (lua_pkgconf_name != null or (lua_libraries.len == 0 and lua_includes.len == 0)) {
                switch (lua_engine) {
                    .LuaJIT => lib.root_module.linkSystemLibrary(lua_pkgconf_name orelse "luajit", .{ .needed = true }),
                    .Lua => lib.root_module.linkSystemLibrary(lua_pkgconf_name orelse "lua", .{ .needed = true }),
                }

                break :system_lua;
            }

            // use the provided libraries and includes
            var includes_it = std.mem.splitScalar(u8, lua_includes, ',');
            while (includes_it.next()) |include_str|
                lib.root_module.addSystemIncludePath(.{ .cwd_relative = include_str });

            var libs_it = std.mem.splitScalar(u8, lua_libraries, ',');
            while (libs_it.next()) |lib_str|
                lib.root_module.linkSystemLibrary(lib_str, .{ .needed = true });
        } else if (want_luarocks_integration) {
            lib.root_module.addIncludePath(.{ .cwd_relative = luarocks_config.variables.LUA_INCDIR });

            if (luarocks_config.link_lua_explicitly) {
                lib.root_module.linkSystemLibrary(b.fmt("{s}/{s}", .{
                    luarocks_config.variables.LUA_LIBDIR,
                    luarocks_config.variables.LUA_LIBDIR_FILE orelse @panic("link_lua_explicitly is true, but LUA_LIBDIR_FILE is not set"),
                }), .{ .needed = true });
            }
        } else {
            if (lua_shared) {
                lib.root_module.linkLibrary(lua.artifact("lua-shared"));
            } else {
                lib.root_module.linkLibrary(lua.artifact("lua-static"));
            }

            // expose lua artifacts to dependents
            b.installArtifact(lua.artifact("lua-shared"));
            b.installArtifact(lua.artifact("lua-static"));
            b.installArtifact(lua.artifact("lua"));
        }

        lib.root_module.addIncludePath(b.dependency("lua_compat_53", .{}).path("c-api"));
        lib.addCSourceFile(.{ .file = b.path("src/luv.c") });
    }

    const test_step = b.step("test", "Run library tests");

    const run_tests = if (want_luarocks_integration)
        b.addSystemCommand(&.{luarocks_config.variables.LUA})
    else if (want_lua_integration)
        b.addSystemCommand(&.{lua_exe})
    else
        b.addRunArtifact(lua.artifact("lua"));
    run_tests.addArg("tests/run.lua");

    run_tests.setEnvironmentVariable("LUA_CPATH", b.fmt("{s}/?{s}", .{ b.lib_dir, target.result.dynamicLibSuffix() }));
    run_tests.has_side_effects = false;

    run_tests.step.dependOn(&install_module.step);
    test_step.dependOn(&run_tests.step);
}

pub const LuaEngine = enum { Lua, LuaJIT };

const std = @import("std");
