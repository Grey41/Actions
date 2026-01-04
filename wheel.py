import subprocess, sysconfig, pathlib, sys, zipfile, shutil, hashlib, base64, packaging.tags, platform

VERSION = "3.1"

def build_wheel(wheel_directory, config_settings = None, metadata_directory = None):
    def write(src, path):
        file.write(src, pathlib.Path("JoBase") / path)

        hash = hashlib.sha256()
        item = open(src, "rb")

        while chunk := item.read(8192):
            hash.update(chunk)

        code = base64.urlsafe_b64encode(hash.digest()).rstrip(b"=").decode("ascii")
        lines.append(f"JoBase/{path},sha256={code},{pathlib.Path(src).stat().st_size}")

    def writestr(path, list):
        text = "\n".join(list)
        bytes = text.encode("utf-8")
        hash = hashlib.sha256()

        file.writestr(path, text)
        hash.update(bytes)

        code = base64.urlsafe_b64encode(hash.digest()).rstrip(b"=").decode("ascii")
        lines.append(f"{path},sha256={code},{len(bytes)}")

    def clone(url, name):
        if not pathlib.Path("lib/" + name).exists():
            subprocess.run(["git", "clone", f"https://github.com/{url}.git", "lib/" + name, "--depth", "1", "--recursive"])

        # elif shutil.which("pacman"): subprocess.run([
        #     "pacman", "-Sy", "--noconfirm",
        #     "libxi",
        #     "libxinerama",
        #     "libxkbcommon",
        #     "libxrandr",
        #     "wayland",
        #     "wayland-protocols"
        # ])

    # if not os.environ.get("JOBASE_DEV"):
    #     # if sys.platform == "darwin" and shutil.which("brew"):
    #     #     subprocess.run(["brew", "install", "opus", "flac", "game-music-emu", "mpg123", "fluidsynth", "wavpack"])

    #     # if sys.platform == "win32":
    #     #     subprocess.run(["vcpkg", "install", "libvorbis", "libflac", "opus", "mpg123", "libxmp", "fluidsynth", "wavpack"])

    base, ext = sysconfig.get_config_vars("installed_base", "EXT_SUFFIX")
    tag = str(next(packaging.tags.sys_tags()))

    wheel = f"JoBase-{VERSION}-{tag}.whl"
    build = "build/" + platform.machine()
    file = zipfile.ZipFile(pathlib.Path(wheel_directory) / wheel, "w")

    lines = []
    cmake = [] if sys.platform != "win32" else ["-A", "x64"] if sys.maxsize > 2 ** 32 else ["-A", "Win32"]

    # cmake = ([] if sys.maxsize > 2 ** 32 or sys.platform != "win32" else ["-A", "Win32"]) + [
    #     f"-DCMAKE_PREFIX_PATH={sdl};{mix}",
    #     "-DCMAKE_OSX_ARCHITECTURES=x86_64;arm64",
    #     "-DCMAKE_OSX_DEPLOYMENT_TARGET=10.13",
    #     "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
    #     "-DCMAKE_BUILD_TYPE=Release",
    #     "-DBUILD_SHARED_LIBS=OFF"
    # ]

    # if not pathlib.Path("lib/sdl/build").exists():
    #     subprocess.run(["cmake", "-S", "lib/sdl", "-B", "lib/sdl/build", *cmake,
    #         "-DSDL_CAMERA=OFF",
    #         "-DSDL_JOYSTICK=OFF",
    #         "-DSDL_HAPTIC=OFF",
    #         "-DSDL_HIDAPI=OFF",
    #         "-DSDL_POWER=OFF",
    #         "-DSDL_SENSOR=OFF",
    #         "-DSDL_DIALOG=OFF",
    #         "-DSDL_TRAY=OFF",
    #         "-DSDL_SHARED=OFF",
    #         "-DSDL_STATIC=ON",
    #         "-DSDL_TESTS=OFF"
    #     ])

    #     subprocess.run(["cmake", "--build", "lib/sdl/build", "--config", "Release"])

    # if not pathlib.Path("lib/mix/build").exists():
    #     subprocess.run(["cmake", "-S", "lib/mix", "-B", "lib/mix/build", *cmake, "-DSDLMIXER_FLAC_LIBFLAC=OFF"])
    #     subprocess.run(["cmake", "--build", "lib/mix/build", "--config", "Release"])

    print("BASE", base)
    print("CMAKE", cmake)
    #"-S", ".", "-B", tag,

    clone("memononen/libtess2", "libtess2")
    clone("libsdl-org/SDL_mixer", "mix")
    clone("nothings/stb", "stb")
    clone("JoBase/SDL", "sdl")

    if sys.platform == "linux":
        if shutil.which("apk"): subprocess.run([
            "apk", "add", "--no-cache",
            "libxi-dev",
            "libxrandr-dev",
            "libxkbcommon-dev",
            "wayland-dev",
            "wayland-protocols",
            "mesa-dev",
            "libdrm-dev"
        ])

        elif shutil.which("dnf"): subprocess.run([
            "dnf", "install", "-y",
            "libXi-devel",
            "libXrandr-devel",
            "libxkbcommon-devel",
            "wayland-devel",
            "wayland-protocols-devel",
            "mesa-libEGL-devel"
        ])

    subprocess.run(["cmake", "-S", ".", "-B", build, *cmake,
        "-DPython3_ROOT_DIR=" + base,
        "-DJOBASE_EXT=" + ext,
        "-DJOBASE_DIR=" + str(pathlib.Path(wheel_directory))
    ])

    subprocess.run(["cmake", "--build", build, "--config", "Release", "--verbose"])
    subprocess.run(["ls", str(pathlib.Path(wheel_directory))])

    for path in pathlib.Path("module").rglob("*"):
        if path.suffix in (".pyi", ".png", ".bin", ".wav"):
            write(path, pathlib.Path(*path.parts[1:]))

    writestr(f"JoBase-{VERSION}.dist-info/METADATA", [
        "Metadata-Version: 2.1",
        "Name: JoBase",
        "Version: " + VERSION,
        "Summary: JoBase is a fast Python game library for beginner coders",
        "Keywords: game engine learn beginner",
        "Home-page: https://jobase.org",
        "Author: Reuben Grey Ford",
        "Author-email: <hello@jobase.org>"
    ])

    writestr(f"JoBase-{VERSION}.dist-info/WHEEL", [
        "Wheel-Version: 1.0",
        "Generator: JoBase " + VERSION,
        "Root-Is-Purelib: false",
        "Tag: " + tag
    ])

    write(pathlib.Path(wheel_directory) / ("__init__" + ext), "__init__" + ext)
    lines.append(f"JoBase-{VERSION}.dist-info/RECORD,,")
    file.writestr(f"JoBase-{VERSION}.dist-info/RECORD", "\n".join(lines))

    return wheel