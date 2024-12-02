# wlink-iap: A tool to flash WCH-Link/E firmware online using IAP method.

With this tool, you don't need to enter ISP mode or buy another WCH-LinkE to flash the firmware of WCH-Link/E.

All known WCH-Link models should be supported, but tested with:

- WCH-Link
- WCH-LinkE r0 1v2
- WCH-LinkE r0 1v3
- Various thirdparty Link/LinkE.

# Installation:

```
git clone https://github.com/cjacker/wlink-iap
cd wlink-iap
make
make install
```

# Usage

```
A tool to upgrade / downgrade WCH-Link/E firmware.
By cjacker <cjacker@gmail.com>

Usage:

wlink-iap -f <firmware file>
  Flash firmware.
  Firmwares can be extracted from WCH-LinkUtility.
  FIRMWARE_CH32V305.bin:  WCH-LinkE(ch32v305)
  FIRMWARE_CH32V208.bin:  WCH-LinkB(ch32v208)
  FIRMWARE_CH32V203.bin:  WCH-LinkS(ch32v203)
  FIRMWARE_CH549.bin:     WCH-Link(ch549) RV
  FIRMWARE_DAP_CH549.bin: WCH-Link(ch549) DAP

wlink-iap -i
  Enter IAP mode and exit.

wlink-iap -q
  Quit IAP mode.
```

## to flash (upgrade or downgrade) WCH-Link/E firmware:

Plug WCH-Link/E to PC USB port and run:

```
./wlink-iap -f <firmware>
```

## to enter IAP mode only

If you want to erase code flash or/and re-program the whole firmware of WCH-LinkE, you have to enter IAP mode first by hold the 'IAP' button down when powerup, or by `wlink-iap -i`.

This command is useful since official WCH-LinkE have a case and it's not convenient to hold the 'IAP' button down.

After IAP mode entered, pleaser refer to [Update firmware of WCH-LinkE](https://github.com/cjacker/opensource-toolchain-ch32v?tab=readme-ov-file#update-firmware-of-wch-linke) to update the whole firmware.

## to quit IAP mode

If WCH-LinkE was switched to IAP mode accidently, for example, user want to switch RV/DAP mode by 'Mode' button but hold 'IAP' button down, You can quit IAP mode by `wlink-iap -q`.


# Firmwares:

I put the WCH-Link(CH549) and WCH-LinkE(CH32V) firmwares in this repo, for other unusual models, you can download and extract firmwares from WCH-LinkUtility.

# Info:

[ghidra](https://github.com/NationalSecurityAgency/ghidra): to analyse binaries of WCH-LinkUtility.

[usbpcap](https://github.com/desowin/usbpcap): to sniff USB data under windows.

# TODO:

Contact [wlink](https://github.com/ch32-rs/wlink) to implement this feature ASAP
