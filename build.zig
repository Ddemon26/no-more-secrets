const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const common_cflags = &[_][]const u8{ "-std=c99", "-Wall", "-Wextra" };

    const nms_module = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });
    const sneakers_module = b.createModule(.{
        .target = target,
        .optimize = optimize,
    });

    const nms = b.addExecutable(.{
        .name = "nms",
        .root_module = nms_module,
    });
    nms.linkLibC();
    nms.addCSourceFiles(.{
        .files = &.{
            "src/error.c",
            "src/input.c",
            "src/nmscharset.c",
            "src/nmseffect.c",
            "src/nms.c",
        },
        .flags = common_cflags,
    });

    const sneakers = b.addExecutable(.{
        .name = "sneakers",
        .root_module = sneakers_module,
    });
    sneakers.linkLibC();
    sneakers.addCSourceFiles(.{
        .files = &.{
            "src/error.c",
            "src/nmscharset.c",
            "src/nmseffect.c",
            "src/sneakers.c",
        },
        .flags = common_cflags,
    });

    if (target.result.os.tag == .windows) {
        nms.addCSourceFiles(.{
            .files = &.{"src/nmstermio_win.c"},
            .flags = common_cflags,
        });
        sneakers.addCSourceFiles(.{
            .files = &.{"src/nmstermio_win.c"},
            .flags = common_cflags,
        });
        nms.linkSystemLibrary("kernel32");
        nms.linkSystemLibrary("user32");
        sneakers.linkSystemLibrary("kernel32");
        sneakers.linkSystemLibrary("user32");
    } else {
        nms.addCSourceFiles(.{
            .files = &.{"src/nmstermio.c"},
            .flags = common_cflags,
        });
        sneakers.addCSourceFiles(.{
            .files = &.{"src/nmstermio.c"},
            .flags = common_cflags,
        });
    }

    b.installArtifact(nms);
    b.installArtifact(sneakers);

    const run_nms_cmd = b.addRunArtifact(nms);
    const run_sneakers_cmd = b.addRunArtifact(sneakers);

    if (b.args) |args| {
        run_nms_cmd.addArgs(args);
        run_sneakers_cmd.addArgs(args);
    }

    run_nms_cmd.step.dependOn(b.getInstallStep());
    const run_nms_step = b.step("run-nms", "Build and run nms");
    run_nms_step.dependOn(&run_nms_cmd.step);

    run_sneakers_cmd.step.dependOn(b.getInstallStep());
    const run_sneakers_step = b.step("run-sneakers", "Build and run sneakers");
    run_sneakers_step.dependOn(&run_sneakers_cmd.step);
}
