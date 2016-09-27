import os

src_dir = "./src"
bin_dir = "./bin"

if not os.path.exists(bin_dir):
    os.makedirs(bin_dir)

command = "g++ -o {}/athens -std=c++11 -Iinclude/".format(bin_dir)

for dirpath, dirnames, filenames in os.walk(src_dir):
    for file in [f for f in filenames]:
        if file.endswith(".cpp"):
            print("{}/{}...".format(dirpath, file))
            command = "{} {}/{} ".format(command, dirpath, file)

os.system("{}".format(command))

print("Build complete")
