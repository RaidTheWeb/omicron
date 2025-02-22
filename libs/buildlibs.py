import json
import shutil
import os

work = { }

if not os.path.isdir("../rt/"):
    os.mkdir("../rt");

with open("work.json") as f:
    work = json.loads(f.read())
    print(work)

    for lib in work:
        print("Building ", lib["name"], "...", sep="")

        cur = os.getcwd();
        os.chdir(lib["path"])

        for instruction in lib["instructions"]:
            os.system(instruction)

        for binary in lib["binaries"]:
            if not os.path.isfile(binary):
                print("Output binary does not exist.")
                exit(1);
            shutil.copyfile(binary, "../../rt/" + os.path.basename(binary))
        os.chdir(cur);

success = open("libsbuilt", "w");
success.write("1");
success.close();
