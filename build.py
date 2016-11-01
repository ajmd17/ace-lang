import os

compiler = ""
if os.name == "nt":
    compiler = "g++"
else:
    compiler = "clang++"

options = "-g"

src_dir = "./src"
bin_dir = "./bin"

if not os.path.exists(bin_dir):
    os.makedirs(bin_dir)

command = "{} {} -o {}/ace-c -std=c++11 -O2 -Iinclude/".format(compiler, options, bin_dir)

for dirpath, dirnames, filenames in os.walk(src_dir):
    for file in [f for f in filenames]:
        if file.endswith(".cpp"):
            print("{}/{}...".format(dirpath, file))
            command = "{} {}/{} ".format(command, dirpath, file)

os.system("{}".format(command))

print("Build complete")
