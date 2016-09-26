import os
import os.path

path = "./src"

# link object files together
linkfile = "g++ -o a.out -std=c++11 -Iinclude/"

for dirpath, dirnames, filenames in os.walk(path):
    for file in [f for f in filenames]:
        if file.endswith(".cpp"):
            linkfile = "{} {}/{} ".format(linkfile, dirpath, file)

# execute command
os.system("{}".format(linkfile))

