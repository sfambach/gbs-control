# PlatformIO pre script: firmware version from git (tag vX.Y.Z -> "X.Y.Z", later commits
# "X.Y.Z-<n>-g<hash>", uncommitted changes "-dirty"). Without git / tags: "dev-<hash>" / "dev".
# Builds outside PlatformIO (Arduino IDE) get the fallback from config.h.
import subprocess

Import("env")


def git(*args):
    try:
        return subprocess.check_output(["git"] + list(args), cwd=env.subst("$PROJECT_DIR"),
                                       stderr=subprocess.DEVNULL).decode().strip()
    except Exception:
        return ""


version = git("describe", "--tags", "--match", "v[0-9]*", "--dirty")
if version.startswith("v"):
    version = version[1:]
else:
    commit = git("describe", "--always", "--dirty")
    version = "dev-" + commit if commit else "dev"

print("GBS_FW_VERSION: " + version)
env.Append(CPPDEFINES=[("GBS_FW_VERSION", '\\"%s\\"' % version)])
