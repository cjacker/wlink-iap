# wlink-iap: A tool to upgrade / downgrade WCH-Link/E firmware online using IAP method.

With this tool, you don't need to enter ISP mode or buy another WCH-LinkE to upgrade / downgrade the firmware of WCH-Link/E.

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
```

# Usage

```
A tool to upgrade / downgrade WCH-Link/E firmware.
By cjacker <cjacker@gmail.com>

Usage: wlink-iap <firmware file>.

Firmwares can be extracted from WCH-LinkUtility.
  FIRMWARE_CH32V305.bin:  WCH-LinkE(ch32v305)
  FIRMWARE_CH32V208.bin:  WCH-LinkB(ch32v208)
  FIRMWARE_CH32V203.bin:  WCH-LinkS(ch32v203)
  FIRMWARE_CH549.bin:     WCH-Link(ch549) RV
  FIRMWARE_DAP_CH549.bin: WCH-Link(ch549) DAP
```

Plug WCH-Link/E to PC USB port, switch to RV mode (by `wlink mode-switch --rv`) and run:

```
./wlink-iap <firmware>
```

# Firmwares:

I put the WCH-Link(CH549) and WCH-LinkE(CH32V) firmwares in this repo, for WCH-LinkS/WCH-LinkB, you can download and extract firmware from WCH-LinkUtility.

# Info:

[ghidra](https://github.com/NationalSecurityAgency/ghidra): to analyse binaries of WCH-LinkUtility.

[usbpcap](https://github.com/desowin/usbpcap): to sniff USB data under windows.

# TODO:

Contact [wlink](https://github.com/ch32-rs/wlink) to implement this feature ASAP
