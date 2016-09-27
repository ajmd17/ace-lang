import os

src = "./src"
bin = "./bin"

if not os.path.exists(bin):
    os.makedirs(bin)

command = "g++ -o {}/athens -std=c++11 -Iinclude/".format(bin)

for dirpath, dirnames, filenames in os.walk(src):
    for file in [f for f in filenames]:
        if file.endswith(".cpp"):
            print("{}/{}...".format(dirpath, file))
            command = "{} {}/{} ".format(command, dirpath, file)

os.system("{}".format(command))

print("Build complete")
