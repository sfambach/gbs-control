# PlatformIO post script (ESP32): also write firmware_merged.bin = bootloader + partition table +
# boot_app0 + app in one image, flashable at 0x0 (esptool, web flashers). firmware.bin stays the app
# only image (OTA updates).
Import("env")


def merge_bin(source, target, env):
    out = env.subst("$BUILD_DIR/firmware_merged.bin")
    images = []
    for offset, image in env.get("FLASH_EXTRA_IMAGES", []):
        images += [offset, image]
    images += [env.subst("$ESP32_APP_OFFSET"), env.subst("$BUILD_DIR/${PROGNAME}.bin")]
    env.Execute(" ".join(['"$PYTHONEXE"', '"$OBJCOPY"', "--chip", env.subst("$BOARD_MCU"), "merge_bin",
                          "--flash_mode", env.subst("${__get_board_flash_mode(__env__)}"),
                          "--flash_size", env.BoardConfig().get("upload.flash_size", "4MB"),
                          "-o", '"%s"' % out] + ['"%s"' % env.subst(i) for i in images]))


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_bin)
