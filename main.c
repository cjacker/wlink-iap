#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>

#include <libusb.h>
#include "errorcode.h"
#include "usb.h"

static const uint16_t wlink_vids[] = {0x1a86, 0};
static const uint16_t wlink_pids[] = {0x8010, 0};
struct libusb_device_handle *wlink_handle = NULL;

//4348:55e0 WinChipHead
static const uint16_t iap_vids[] = {0x4348, 0};
static const uint16_t iap_pids[] = {0x55e0, 0};
struct libusb_device_handle *iap_handle = NULL;


// return true if the file specified
// by the filename exists
int file_exists(const char *filename)
{
    struct stat buffer;
    return stat(filename, &buffer) == 0;
}


// win32 Sleep(1) == linux usleep(1000);

int main(int argc, char **argv)
{
  if(argc != 2) {
    printf("A tool to upgrade / downgrade WCH-Link/E firmware.\n");
    printf("By cjacker <cjacker@gmail.com>\n");
    printf("\nUsage: wlink-iap <firmware file>.\n");
    printf("\nFirmwares can be extracted from WCH-LinkUtility.\n");
    printf("  FIRMWARE_CH32V305.bin:  WCH-LinkE(ch32v305)\n");
    printf("  FIRMWARE_CH32V208.bin:  WCH-LinkB(ch32v208)");
    printf("  FIRMWARE_CH32V203.bin:  WCH-LinkS(ch32v203)");
    printf("  FIRMWARE_CH549.bin:     WCH-Link(ch549) RV\n");
    printf("  FIRMWARE_DAP_CH549.bin: WCH-Link(ch549) DAP\n");
    exit(0);
  }

  // check firmware file exist or not.
  char *firmware_file = strdup(argv[1]);
  if(!file_exists(firmware_file)) {
    fprintf(stderr,"Firmware \"%s\" not found!\n", firmware_file);
    exit(1);
  }


  unsigned char txbuf[6];
  unsigned char rxbuf[20];
  unsigned int len = 5;

  int ret;

  if (jtag_libusb_open(wlink_vids, wlink_pids, &wlink_handle) != ERROR_OK)
  {
    fprintf(stderr, "open wlink device failed\n");
    goto end;
  }

  jtag_libusb_set_configuration(wlink_handle, 0);

  if (libusb_claim_interface(wlink_handle, 0) != ERROR_OK)
  {
    fprintf(stderr, "claim wlink interface failed\n");
    goto end;
  }

  // step 1: Read adatper type and firmware version
  txbuf[0] = 0x81;
  txbuf[1] = 0x0d;
  txbuf[2] = 0x01;
  txbuf[3] = 0x01;
  len = 4;
  pWriteData(wlink_handle, 1, txbuf, &len);
  len = 7;
  pReadData(wlink_handle, 1, rxbuf, &len);
  printf("Adapter type:%x, version: v%d.%d\n", rxbuf[5], rxbuf[3], rxbuf[4]);

  // should force validate the firmware file name.
  switch (rxbuf[5]) {
    case 1: 
      printf("Found WCH-Link(ch549)\n");
      break;
    case 2:
    case 0x12:
      printf("Found WCH-LinkE(ch32v305)\n");
      break;
    case 3:
      printf("Found WCH-LinkS(ch32v203)\n");
      break;
    case 5:
    case 0x85:
      printf("Found WCH-LinkB(ch32v208)\n");
      break;
    default:
      printf("Not tested and not supported adapter\n");
      goto end;
  }

  printf("Start flash the firmware...\n");
  printf("###########################\n");
  printf("DO NOT UNPLUG WCH-Link/E until done!!!\n");
  printf("###########################\n");

  // according to adapter type, find wchlinke chiptype and correct firmware
  // and compare version info with "wchlink.wcfg"
  // for adapter type 0x02 and 0x12, it's WCH-LinkE: FIRMWARE_CH32V203.BIN  
  
  // if outdated:
  // step 2: load firmware to buffer 
  //0x80000 is from McuCompilerDLL.dll

  /* declare a file pointer */
  FILE    *infile;
  char    *buffer;
  long    filesize;

  /* open an existing file for reading */
  infile = fopen(firmware_file, "r");

  /* quit if the file does not exist */
  if(infile == NULL)
      return 1;

  /* Get the number of bytes */
  fseek(infile, 0L, SEEK_END);
  filesize = ftell(infile);

  /* reset the file position indicator to
  the beginning of the file */
  fseek(infile, 0L, SEEK_SET);

  /* grab sufficient memory for the
  buffer to hold the text */
  buffer = (char*)calloc(filesize, sizeof(char));

  /* memory error */
  if(buffer == NULL)
      return 1;

  /* copy all the text into the buffer */
  fread(buffer, sizeof(char), filesize, infile);
  fclose(infile);

  // step3: switch to IAP mode: 0x01010f81 len 4
  txbuf[0] = 0x81;
  txbuf[1] = 0x0f;
  txbuf[2] = 0x01;
  txbuf[3] = 0x01;
  len = 4;
  ret = pWriteData(wlink_handle, 1, txbuf, &len);

  if(ret != ERROR_OK) {
    fprintf(stderr, "Set IAP mode failed\n");
    goto end;
  }

  // win32 Sleep(0xdac);
  usleep(3500*1000);

  if (jtag_libusb_open(iap_vids, iap_pids, &iap_handle) != ERROR_OK)
  {
    fprintf(stderr, "Open iap device failed\n");
    goto end;
  }

  jtag_libusb_set_configuration(iap_handle, 0);

  if(libusb_kernel_driver_active(iap_handle, 0) == 1)
  {
    if(libusb_detach_kernel_driver(iap_handle, 0) != 0)
      fprintf(stderr, "Couldn't detach kernel driver!\n");
  }

  ret = libusb_claim_interface(iap_handle, 0);
  if (ret !=  LIBUSB_SUCCESS) {
    fprintf(stderr, "Claim interface error: %s", libusb_error_name(ret));
    goto end;
  }


  // before program
  txbuf[0] = 0x81;
  txbuf[1] = 0x02;
  txbuf[2] = 0x00;
  txbuf[3] = 0x00;
  len = 4;
  ret = pWriteData(iap_handle, 2, txbuf, &len);
  if(ret != ERROR_OK) {
    fprintf(stderr, "Prepare failed\n");
    goto end;
  }
  
  usleep(1000);

  len = 2;
  ret = pReadData(iap_handle, 2, rxbuf, &len);
  if(ret != ERROR_OK) {
    fprintf(stderr, "Prepare failed\n");
    goto end;
  }
  // should be 0x0000
  if(rxbuf[0] != 0 || rxbuf[1] != 0)
    goto end;


  // if not ch549 and rxbuf[0]= 0x00, rxbuf[1] = 0x00
  // start write firmware.
  char small_buf[64];
 
  // this is first time write. 
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

    //first time write.
    small_buf[0] = 0*2 + 0x80;
    small_buf[1] = copy_size;
    small_buf[2] = offset;
    small_buf[3] = offset >>8;
    memcpy(small_buf+4, buffer + offset, copy_size);

    ret = pWriteData(iap_handle, 2, small_buf, &len);

    if(ret != ERROR_OK) {
      fprintf(stderr, "Firmware write error\n");
      goto end;
    }

    usleep(1000);
    
    len = 2;
    ret = pReadData(iap_handle, 2, rxbuf, &len);
    if(ret != ERROR_OK || rxbuf[0]!= 0x00 || rxbuf[1] != 0x00) {
      fprintf(stderr, "Reply error\n");
      goto end;
    }

		offset = offset + copy_size;
  }


  // this is second time write. 
  offset = 0;
  copy_size = 60;
  len = copy_size + 4;

  while(1) {
    
    if (offset >= filesize)
      break;

    if (offset+copy_size > filesize) {
      copy_size = filesize - offset;
    }

    len = copy_size + 4;

    memset(small_buf, 0, 64);

    //second time write.
    //note the: 1*2
    small_buf[0] = 1*2 + 0x80;
    small_buf[1] = copy_size;
    small_buf[2] = offset;
    small_buf[3] = offset >>8;
    memcpy(small_buf+4, buffer + offset, copy_size);

    ret = pWriteData(iap_handle, 2, small_buf, &len);

    if(ret != ERROR_OK) {
      fprintf(stderr, "Firmware write error\n");
      goto end;
    }

    usleep(1000);
    
    len = 2;
    ret = pReadData(iap_handle, 2, rxbuf, &len);
    
    if(ret != ERROR_OK || rxbuf[0]!= 0x00 || rxbuf[1] != 0x00) {
      fprintf(stderr, "Reply error\n");
      goto end;
    }

		offset = offset + copy_size;
  }

  // after program, quit IAP mode 
  txbuf[0] = 0x83;
  txbuf[1] = 0x02;
  txbuf[2] = 0x00;
  txbuf[3] = 0x00;
  len = 4;
  ret = pWriteData(iap_handle, 2, txbuf, &len);
  if(ret != ERROR_OK) {
    fprintf(stderr, "Quit IAP mode failed\n");
    goto end;
  }

  printf("Done!\n");
 end:
  //if(wlink_handle) 
  //  jtag_libusb_close(wlink_handle); 
  if(iap_handle) 
    jtag_libusb_close(iap_handle); 
  return 0;
}


