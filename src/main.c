#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>

#include <libusb.h>
#include "usb.h"

// for arguments
#include "arg.h"
char *argv0;

// ret 0 if file exists
int file_not_exists(const char *filename)
{
    struct stat buffer;
    return stat(filename, &buffer);
}

// load file with filename to buffer and set filesize.
int load_firmware(char *filename, char **buffer, long *filesize) {
  FILE    *infile;

  infile = fopen(filename, "r");

  if(infile == NULL)
      return 1;

  /* Get the number of bytes */
  fseek(infile, 0L, SEEK_END);
  *filesize = ftell(infile);

  /* reset the file position indicator to
  the beginning of the file */
  fseek(infile, 0L, SEEK_SET);

  *buffer = (char*)calloc(*filesize, sizeof(char));

  if(*buffer == NULL)
      return 1;

  /* copy all the text into the buffer */
  fread(*buffer, sizeof(char), *filesize, infile);
  fclose(infile);
  return 0; 
}

// value for last param 'cmd':
// 0x80 to download
// 0x82 to verify
int flash_firmware(struct libusb_device_handle * iap_handle,
    char *buffer, long filesize, u_int8_t cmd)
{
  unsigned char txbuf[6];
  unsigned char rxbuf[20];
  unsigned int len = 0;
  unsigned int ret;

  unsigned char small_buf[64];
  int offset = 0;
  int copy_size = 60;
  len = copy_size + 4;

 
  while(1) {
    
    if (offset >= filesize)
      break;

    if (offset+copy_size > filesize) {
      copy_size = filesize - offset;
    }

    len = copy_size + 4;

    memset(small_buf, 0, 64);

    small_buf[0] = cmd;
    small_buf[1] = copy_size;
    small_buf[2] = offset;
    small_buf[3] = offset >>8;
    memcpy(small_buf+4, buffer + offset, copy_size);

    ret = pWriteData(iap_handle, 2, small_buf, &len);

    if(ret != 0) {
      fprintf(stderr, "Firmware write error\n");
      return ret;
    }

    usleep(1000);
    
    len = 2;
    ret = pReadData(iap_handle, 2, rxbuf, &len);
    if(ret != 0 || rxbuf[0]!= 0x00 || rxbuf[1] != 0x00) {
      fprintf(stderr, "Reply error\n");
      return ret;
    }

    offset = offset + copy_size;
  }
  return 0;
}

// open usb device and return a handle
// sucess ret 0, fail ret 1
int open_device(const uint16_t vids[],
    const uint16_t pids[],
    struct libusb_device_handle **handle) {
  if (jtag_libusb_open(vids, pids, handle) != 0)
  {
    fprintf(stderr, "Fail to open usb device.\n");
		return 1;
  }

  jtag_libusb_set_configuration(*handle, 0);

  if(libusb_kernel_driver_active(*handle, 0) == 1)
  {
    if(libusb_detach_kernel_driver(*handle, 0) != 0)
      fprintf(stderr, "Couldn't detach kernel driver!\n");
  }

  int ret = libusb_claim_interface(*handle, 0);
  if(ret != LIBUSB_SUCCESS) {
    fprintf(stderr, "Claim interface failed: %s", libusb_error_name(ret));
		return 1;
  }
  return 0;
}


int switch_to_iap_mode(struct libusb_device_handle *wlink_handle,
    int endp_out) {
  unsigned char txbuf[6];
  unsigned char rxbuf[20];
  unsigned int len = 5;
	int ret;

  txbuf[0] = 0x81;
  txbuf[1] = 0x0f;
  txbuf[2] = 0x01;
  txbuf[3] = 0x01;
  len = 4;
  ret = pWriteData(wlink_handle, endp_out, txbuf, &len);
  if(ret != 0) {
		return 1;
  }

  usleep(1000);

  if(device_exists(0x4348, 0x55e0))
    return 0;

  return 1;
}

int erase_app_flash(struct libusb_device_handle *iap_handle) {
  unsigned char txbuf[6];
  unsigned char rxbuf[20];
  unsigned int len = 5;
	int ret;

  txbuf[0] = 0x81;
  txbuf[1] = 0x02;
  txbuf[2] = 0x00;
  txbuf[3] = 0x00;
  len = 4;

  ret = pWriteData(iap_handle, 2, txbuf, &len);
  if(ret != 0)
		return 1;

  usleep(1000);

  len = 2;
  ret = pReadData(iap_handle, 2, rxbuf, &len);
  if(ret != 0)
		return 1;
  // expect 0x00, 0x00
  if(rxbuf[0] != 0 || rxbuf[1] != 0)
		return 1;

	return 0;
}

int quit_iap_mode(struct libusb_device_handle *iap_handle) {
  unsigned char txbuf[6];
  unsigned char rxbuf[20];
  unsigned int len = 5;
	int ret;

  txbuf[0] = 0x83;
  txbuf[1] = 0x02;
  txbuf[2] = 0x00;
  txbuf[3] = 0x00;
  len = 4;
  ret = pWriteData(iap_handle, 2, txbuf, &len);
  if(ret != 0)
		return 1;
	return 0;
}

// Adapter type and version info can be used to
// 1, find the correct firmware.
// 2, compare version with wchlink.cfg and determine update or not.
// Here only display to screen.
//
// It's not neccesary to compare the version since wlink-iap support 
// downgrade the firmware. 
int get_wchlink_info(struct libusb_device_handle *wlink_handle,
    int endp_out, int endp_in) {
  unsigned char txbuf[6];
  unsigned char rxbuf[20];
  unsigned int len = 5;
	int ret;

  txbuf[0] = 0x81;
  txbuf[1] = 0x0d;
  txbuf[2] = 0x01;
  txbuf[3] = 0x01;
  len = 4;
  ret = pWriteData(wlink_handle, endp_out, txbuf, &len);

  if(ret != 0)
    return 1;

  usleep(1000);

  len = 7;
  ret = pReadData(wlink_handle, endp_in, rxbuf, &len);
  if(ret != 0)
    return 1;

  printf("Found device : ");

  switch (rxbuf[5]) {
    case 1: 
      printf("WCH-Link(ch549)");
      break;
    case 2:
    case 0x12:
      printf("WCH-LinkE(ch32v305)");
      break;
    case 3:
      printf("WCH-LinkS(ch32v203)");
      break;
    case 5:
    case 0x85:
      printf("WCH-LinkB(ch32v208)");
      break;
    default:
      printf("Unknown adapter\n");
      return 1;
  }
 
  // display version
  // Version in wchlink.wcfg is "rxbuf[3]*10 + rxbuf[4]"
  // But in wch-openocd, it's "rxbuf[3].rxbuf[4]"
  printf(" with firmware v%d.%d\n", rxbuf[3], rxbuf[4]);

  return 0;
}


void usage()
{
    printf("A tool to flash WCH-Link/E firmware.\n");
    printf("By cjacker <cjacker@gmail.com>\n\n");
    printf("Usage:\n\n");
    printf("wlink-iap -f <firmware file>\n");
    printf("  Flash firmware.\n");
    printf("  Firmwares can be extracted from WCH-LinkUtility.\n");
    printf("  FIRMWARE_CH32V305.bin:  WCH-LinkE(ch32v305)\n");
    printf("  FIRMWARE_CH549.bin:     WCH-Link(ch549) RV\n");
    printf("  FIRMWARE_DAP_CH549.bin: WCH-Link(ch549) DAP\n");
    printf("  FIRMWARE_CH32V208.bin:  WCH-LinkB(ch32v208)\n");
    printf("  FIRMWARE_CH32V203.bin:  WCH-LinkS(ch32v203)\n\n");
    printf("wlink-iap -i\n");
    printf("  Enter IAP mode.\n\n");
    printf("wlink-iap -q\n");
    printf("  Quit IAP mode.\n\n");
    exit(0);
}

int main(int argc, char **argv)
{
  // no args, display usage and exit
  if(argc == 1) {
    usage();
    exit(0);
  }
  
  char *firmware_file = NULL;

  // flag to switch to IAP only.
  int into_iap = 0;
  // flag to quit IAP only.
  int quit_iap = 0;

  ARGBEGIN {
  case 'f':
    if(into_iap == 1 || quit_iap == 1) {
      fprintf(stderr, "Don't use '-f' with '-i' or '-q'\n");
      exit(1);
    }
    firmware_file = EARGF(usage());
    break;
  case 'i':
    if(firmware_file || quit_iap == 1) {
      fprintf(stderr, "Don't use '-i' with '-q' or '-f'\n");
      exit(1);
    }
    into_iap = 1;
    break;
  case 'q':
    if(firmware_file || into_iap == 1) {
      fprintf(stderr, "Don't use '-q' with '-i' or '-f'\n");
      exit(1);
    }
    quit_iap = 1;
    break;
  case 'h':
    usage();
    exit(0);
  default:
    usage();
  }
  ARGEND;

  // Check device existance
  if(device_exists(0x1a86, 0x8010) != 0 &&
		  device_exists(0x1a86, 0x8012) != 0 &&
		  device_exists(0x4348, 0x55e0) != 0) {
    fprintf(stderr, "WCH-Link/E device not found\n");
    exit(1);
  }

  // If found 0x4348, 0x55e0, it maybe a wchlink already in IAP mode,
  // And maybe some other devices we don't know.
  // Let user try to quit IAP mode first.
  if(device_exists(0x4348, 0x55e0) == 0 && quit_iap != 1) {
    printf("Maybe WCH-Link/E already in IAP mode, but I'm not sure.\n");
    printf("Please try to quit IAP mode first by:\n\n");
    printf("wlink-iap -q\n");
    exit(0);
  }

  // only check file existance here.
  // check it early to better handle into_iap == 1,
  // avoid load firmware before switch to IAP mode.
  if(quit_iap == 0 && into_iap == 0 && 
      file_not_exists(firmware_file)) {
    fprintf(stderr,"Firmware \"%s\" not found!\n", firmware_file);
    exit(1);
  }

  //0x1a86:0x8010 RV mode
  //0x1a86:0x8012 DAP mode
  const uint16_t wlink_vids[] = {0x1a86, 0x1a86, 0};
  const uint16_t wlink_pids[] = {0x8010, 0x8012, 0};
  struct libusb_device_handle *wlink_handle = NULL;


  //4348:55e0 WinChipHead
  const uint16_t iap_vids[] = {0x4348, 0};
  const uint16_t iap_pids[] = {0x55e0, 0};
  struct libusb_device_handle *iap_handle = NULL;

  // setup endp_out/in for RV mode.
  int endp_out = 1;
  int endp_in = 1;
 
  // setup endp_out/in for DAP mode.
  if(device_exists(0x1a86, 0x8012) == 0) {
    endp_out = 2;
    endp_in = 3;
  } 
  
  // with '-q', quit IAP mode directly.
  if(quit_iap == 1) {
    //open IAP device.
    if(open_device(iap_vids, iap_pids, &iap_handle) != 0) {
      if(iap_handle)
        jtag_libusb_close(iap_handle);
        exit(1);
    }

    //send quit command.
    printf("Quit IAP mode \n");
    if(quit_iap_mode(iap_handle) != 0) 
      printf("Failed\n");
    else
      printf("Done\n");

    //clean up.
    if(iap_handle)
        jtag_libusb_close(iap_handle);

		exit(0);
  } 


  int ret;
  // open wlink device.
  ret = open_device(wlink_vids, wlink_pids, &wlink_handle);

  if(ret != 0) {
    fprintf(stderr, "Faile to open device.\n");
    if(wlink_handle)
      jtag_libusb_close(wlink_handle);
    exit(1);
  } 
  

  // STEP 1: Read adatper type and firmware version
  ret = get_wchlink_info(wlink_handle, endp_out, endp_in);
  if(ret != 0) {
    fprintf(stderr, "Fail to get device info.\n");
    if(wlink_handle)
      jtag_libusb_close(wlink_handle);
    exit(1);
  }
  
  // STEP 2: switch to IAP mode: 0x01010f81 len 4
  printf("Switch to IAP mode.\n");
  ret = switch_to_iap_mode(wlink_handle, endp_out);
  if(ret != 0) {
    fprintf(stderr,"Fail to switch to IAP mode.\n");
    exit(1);
  }
 
  // if 'wlink-iap -i'
  if(into_iap == 1) {
    printf("Done!\n");
    exit(0);
  } 
 
  // The reason I put it after switch_to_iap_mode is:
  // If 'into_iap' only, the filename will be null.
  //
  // STEP 3: load firmware to buffer and get filesize.
  long filesize = 0;
  char *buffer = NULL; 

  ret = load_firmware(firmware_file, &buffer, &filesize);
  if(ret != 0) {
    fprintf(stderr, "Fail to load firmware file.\n");
    exit(1);
  }

  // READY, go
  printf("Start flash the firmware...\n");
  printf("######################################\n");
  printf("DO NOT UNPLUG WCH-Link/E until done!!!\n");
  printf("######################################\n");

  // win32 Sleep(1) == linux usleep(1000);
  // win32 Sleep(0xdac);
  // ch549 need to wait for a long time to enter IAP mode.
  usleep(3500*1000);

  // STEP 4: open IAP device
  ret = open_device(iap_vids, iap_pids, &iap_handle);

  if(ret != 0) {
    fprintf(stderr, "Fail to open IAP device.\n");
    goto end;
  }

  // STEP 5: erase the app flash area.
  ret = erase_app_flash(iap_handle);
  if(ret != 0) {
    fprintf(stderr, "Fail to erase.\n");
    goto end;
  }
  
  // STEP 6: first loop, download the firmware.
  ret = flash_firmware(iap_handle, buffer, filesize, 0x80);
  if(ret != 0) {
		fprintf(stderr,"Fail to download firmware.\n");
    goto end;
  }

  // STEP 7: second loop, verify the firmware
  ret = flash_firmware(iap_handle, buffer, filesize, 0x82);
  if(ret != 0) {
		fprintf(stderr,"Fail to verify firmware.\n");
    goto end;
  }
  
  // STEP 8: After program, quit IAP mode 
  ret = quit_iap_mode(iap_handle);
  if(ret != 0) {
		fprintf(stderr,"Fail to quit IAP mode.\n");
    goto end;
  }
  
  printf("Done!\n");

end: 
  if(buffer)
    free(buffer);
  if(iap_handle) 
    jtag_libusb_close(iap_handle);
}


