#!/usr/bin/env python
import os
import sys

from methods import print_error
import methods
import glsl_builders
from SCons.Variables import PathVariable

libname = "chunkinator"

localEnv = Environment(tools=["default"], PLATFORM="")

# Build profiles can be used to decrease compile times.
# You can either specify "disabled_classes", OR
# explicitly specify "enabled_classes" which disables all other classes.
# Modify the example file as needed and uncomment the line below or
# manually specify the build_profile parameter when running SCons.

# localEnv["build_profile"] = "build_profile.json"

customs = ["custom.py"]
customs = [os.path.abspath(path) for path in customs]

opts = Variables(customs, ARGUMENTS)

opts.Add(
    PathVariable(
        key="project_dir",
        help="Path to a project dir to copy the final binaries to",
        default=localEnv.get("project_dir", None),
    )
)

opts.Update(localEnv)

projectdir = ARGUMENTS.get("project_dir", None)

Help(opts.GenerateHelpText(localEnv))

env = localEnv.Clone()

env.__class__.Run = methods.Run

GLSL_BUILDERS = {
    "RD_GLSL": Builder(
        action=Action(glsl_builders.build_rd_headers, "$GENCOMSTR"),
        suffix="glsl.gen.h",
        src_suffix=".glsl",
    ),
    "GLSL_HEADER": env.Builder(
        action=env.Run(glsl_builders.build_raw_headers),
        suffix="glsl.gen.h",
        src_suffix=".glsl",
    ),
}

env = SConscript("godot-cpp/SConstruct", {"env": env, "customs": customs})

env.Append(CPPPATH=["src/"])
sources = Glob("src/*.cpp")
sources += Glob("src/debugger/*.cpp")
sources += Glob("src/chunkinator/*.cpp")
sources += Glob("src/terrain_generator/*.cpp")
sources += Glob("src/console/*.cpp")
sources += Glob("src/console/gui/*.cpp")
sources += Glob("src/game/*.cpp")
sources += Glob("src/debug/*.cpp")
sources += Glob("src/animation/*.cpp")
sources += Glob("src/game/ui/*.cpp")
sources += Glob("src/vehicle/*.cpp")
sources += Glob("src/vehicle/telemetry/*.cpp")
sources += Glob("src/game/turret/*.cpp")
sources += Glob("src/game/rexbot/*.cpp")
sources.append("/mnt/data_drive/porter/tracy/public/TracyClient.cpp")

env.Append(BUILDERS=GLSL_BUILDERS)

if not (os.path.isdir("godot-cpp") and os.listdir("godot-cpp")):
    print_error("""godot-cpp is not available within this folder, as Git submodules haven't been initialized.
Run the following command to download godot-cpp:

    git submodule update --init --recursive""")
    sys.exit(1)

if "RD_GLSL" in env["BUILDERS"]:
    # find just the include files
    gl_include_files = [str(f) for f in Glob("src/shaders/*_inc.glsl")]

    # find all shader code (all glsl files excluding our include files)
    glsl_files = [
        str(f) for f in Glob("src/shaders/*.glsl") if str(f) not in gl_include_files
    ]

    # make sure we recompile shaders if include files change
    env.Depends(
        [f + ".gen.h" for f in glsl_files], gl_include_files + ["#glsl_builders.py"]
    )

    # compile include files
    for glsl_file in gl_include_files:
        env.GLSL_HEADER(glsl_file)

    # compile RD shader
    for glsl_file in glsl_files:
        import os

        print("TRYBUILD", os.path.isfile(glsl_file))
        env.RD_GLSL(glsl_file)

if env["target"] in ["editor", "template_debug"]:
    try:
        doc_data = env.GodotCPPDocData(
            "src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml")
        )
        sources.append(doc_data)
    except AttributeError:
        print("Not including class reference as we're targeting a pre-4.3 baseline.")

# .dev doesn't inhibit compatibility, so we don't need to key it.
# .universal just means "compatible with all relevant arches" so we don't need to key it.
suffix = env["suffix"].replace(".dev", "").replace(".universal", "")

lib_filename = "{}{}{}{}".format(
    env.subst("$SHLIBPREFIX"), libname, suffix, env.subst("$SHLIBSUFFIX")
)

library = env.SharedLibrary(
    "bin/{}/{}".format(env["platform"], lib_filename),
    source=sources,
)

if env.get("is_msvc", False):
    env.Append(CXXFLAGS=["/std:c++20"])
else:
    env.Append(CXXFLAGS=["-std=c++20"])

env.Append(CPPDEFINES=["TRACY_ENABLE"])
env.Append(CPPPATH=["/mnt/data_drive/porter/tracy/public"])

if projectdir != None:
    copy = env.Install("{}/bin/{}/".format(projectdir, env["platform"]), library)
    default_args = [library, copy]
else:
    default_args = [library]

Default(*default_args)
