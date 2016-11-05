import os
import sys

compiler = "clang++"
dyn_ext = "so"

lib_options = "-std=c++11 -fPIC"
exec_options = "-std=c++11"

if os.name == "nt":
    compiler = "g++"
    dyn_ext = "dll"
    exec_options = "-std=gnu++11"
    lib_options = "-std=gnu++11"
elif sys.platform == "darwin":
    compiler = "clang++"
    dyn_ext = "dylib"


def build_all(build_mode):
    if build_mode == 0:
        # build each project as a standalone executable.
        build_project("ace-c", 0)
        build_project("ace-vm", 0)
    elif build_mode == 1:
        build_project("ace-c", 1)
        build_project("ace-vm", 1)
        build_project("ace", 0, ["ace-c", "ace-vm"])
    else:
        print("Unknown build mode '{}'".format(build_mode))

def build_project(project_name, build_mode, linkfiles=[]):
    answer = raw_input("Build project '{}'? (Y/n) ".format(project_name))
    if not answer.lower().startswith("y"):
        print("Cancelled")
        return

    sys.stdout.write("Building project {}...".format(project_name))
    sys.stdout.flush()

    if not os.path.exists("./bin"):
        os.makedirs("./bin")

    src_dir = "./src/{}".format(project_name)

    if build_mode == 0:
        bin_file = "./bin/{}".format(project_name)
        build_executable(src_dir, bin_file, linkfiles)
    elif build_mode == 1:
        bin_file = "./bin/lib{}.{}".format(project_name, dyn_ext)
        build_dynamic(src_dir, bin_file)

    print("complete")


def build_executable(src_dir, bin_file, linkfiles):
    linkstr = ""
    if len(linkfiles) != 0:
        linkstr = "-Lbin/"
        for f in linkfiles:
            linkstr += " -l{}".format(f)

    command = "{} {} -o {} -O2 -Iinclude/".format(compiler, exec_options, bin_file)

    for dirpath, dirnames, filenames in os.walk(src_dir):
        for filename in [f for f in filenames]:
            if filename.endswith(".cpp"):
                command += " {}/{} ".format(dirpath, filename)

    linkstr += " -Wl,-rpath='$ORIGIN'"
    command += " {}".format(linkstr)

    os.system("{}".format(command))


def build_dynamic(src_dir, bin_file):
    command = "{} -shared {} -o {} -O2 -Iinclude/".format(compiler, lib_options, bin_file)

    for dirpath, dirnames, filenames in os.walk(src_dir):
        for filename in [f for f in filenames]:
            if filename.endswith(".cpp") and filename != "main.cpp":
                command += " {}/{} ".format(dirpath, filename)

    os.system("{}".format(command))


# 0 - Build executable ace-c and ace-vm.
# 1 - Build ace-c and ace-vm as dynamic libs, and build ace as executable.
build_mode = 1
build_all(build_mode)
