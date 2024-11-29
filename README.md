# wlink-iap: A tool to upgrade / downgrade WCH-LinkE firmware online using IAP method.

Just like WCH-LinkUtility, this is the tool to upgrade or downgrade WCH-LinkE firmware online using IAP method.

With this tool, you don't need to enter ISP mode or buy another WCH-LinkE to upgrade / downgrade the firmware of WCH-LinkE.

Tested with:

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

# Usage:

Plug WCH-Link to PC USB port and run:

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
