import os
import sys

compiler = ""
if os.name == "nt":
    compiler = "g++"
else:
    compiler = "clang++"

options = ""

def build_project(project_name):
    src_dir = "./src/{}".format(project_name)
    bin_dir = "./bin"

    sys.stdout.write("Building project {}...".format(project_name))
    sys.stdout.flush()

    if not os.path.exists(bin_dir):
        os.makedirs(bin_dir)

    command = "{} {} -o {}/{} -std=c++11 -O2 -Iinclude/".format(compiler, options, bin_dir, project_name)

    for dirpath, dirnames, filenames in os.walk(src_dir):
        for filename in [f for f in filenames]:
            if filename.endswith(".cpp"):
                command += " {}/{} ".format(dirpath, filename)

    os.system("{}".format(command))
    print("complete")


projects = ["ace-c", "ace-vm"]
for project in projects:
    answer = raw_input("Build project '{}'? (Y/n) ".format(project))
    if answer.lower().startswith("y"):
        build_project(project)
