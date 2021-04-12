
import subprocess
import re

def multiprocess(args):
    PATH = "/home/user/SysOps/lab3/obj/z2"
    args = [":".join(arg) for arg in args]

    args = [PATH] + args

    proc = subprocess.run(args, capture_output=True)
    if proc.returncode != 0:
        raise RuntimeError(f"{proc} failed")

    stdout = proc.stdout.decode("utf-8")

    real = re.search(r"real\s+time:\s+(\d+)", stdout)
    real = int(real.group(1))

    user = re.search(r"user\s+time:\s+(\d+)", stdout)
    user = int(user.group(1))

    sys = re.search(r"sys\s+time:\s+(\d+)", stdout)
    sys = int(sys.group(1))

    return real, user, sys

def singleprocess(args):
    PATH = "/home/user/SysOps/lab1/obj/zad2.static.o3"

    args = [":".join(arg) for arg in args]
    args = [PATH, "create_table", str(len(args) * 2), "merge_files"] + args

    proc = subprocess.run(args, capture_output=True)
    if proc.returncode != 0:
        raise RuntimeError(f"{proc} failed")

    stdout = proc.stdout.decode("utf-8")

    real = re.search(r"total real\s+time:\s+(\d+)", stdout)
    real = int(real.group(1))

    user = re.search(r"total user\s+time:\s+(\d+)", stdout)
    user = int(user.group(1))

    sys = re.search(r"total sys\s+time:\s+(\d+)", stdout)
    sys = int(sys.group(1))

    return real, user, sys

def main():

    with open("empty.txt", "w") as f:
        pass

    with open("rows10.txt", "w") as f:
        for _ in range(10):
            f.write("A" * 100)

    with open("rows1000.txt", "w") as f:
        for _ in range(100):
            f.write("A" * 100)

    with open("rows100000.txt", "w") as f:
        for _ in range(100000):
            f.write("A" * 100)

    sets = {
        "empty.txt * 1": [("empty.txt", "empty.txt")] * 1,
        "empty.txt * 100": [("empty.txt", "empty.txt")] * 100,
        "empty.txt * 10000": [("empty.txt", "empty.txt")] * 10000,

        "rows10.txt * 1": [("rows10.txt", "rows10.txt")] * 1,
        "rows10.txt * 100": [("rows10.txt", "rows10.txt")] * 100,
        "rows10.txt * 10000": [("rows10.txt", "rows10.txt")] * 10000,

        "rows1000.txt * 1": [("rows1000.txt", "rows1000.txt")] * 1,
        "rows1000.txt * 100": [("rows1000.txt", "rows1000.txt")] * 100,
        "rows1000.txt * 10000": [("rows1000.txt", "rows1000.txt")] * 10000,

        "rows100000.txt * 1": [("rows100000.txt", "rows100000.txt")] * 1,
        "rows100000.txt * 50": [("rows100000.txt", "rows100000.txt")] * 50,
        "rows100000.txt * 100": [("rows100000.txt", "rows100000.txt")] * 100,
    }

    for name, args in sets.items():
        print(name)
        print("\tsingle", singleprocess(args))
        print("\tmulti", multiprocess(args))


if __name__ == "__main__":
    main()

