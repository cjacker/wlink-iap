#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/types.h>
#include <string.h>
#include <libusb.h>
#include "usb.h"

int jtag_libusb_bulk_write(struct libusb_device_handle *dev, int ep, unsigned char *bytes,
                           int size, int timeout, int *transferred)
{
  int ret;

  *transferred = 0;

  ret = libusb_bulk_transfer(dev, ep, (unsigned char *)bytes, size,
			     transferred, timeout);
  if (ret != LIBUSB_SUCCESS) {
    printf("libusb_bulk_write error: %s", libusb_error_name(ret));
    return ret;
  }

  return 0;
}

int jtag_libusb_bulk_read(struct libusb_device_handle *dev, int ep, unsigned char *bytes,
                          int size, int timeout, int *transferred)
{
  int ret;

  *transferred = 0;

  ret = libusb_bulk_transfer(dev, ep, (unsigned char *)bytes, size,
			     transferred, timeout);
  if (ret != LIBUSB_SUCCESS) {
    printf("libusb_bulk_read error: %s", libusb_error_name(ret));
    return ret;
  }

  return 0;
}


int pWriteData(libusb_device_handle *handle, int endpoint, unsigned char *buf, unsigned int *length)
{
  int ret, pr;
  length = (unsigned int *)length;
  ret = jtag_libusb_bulk_write(handle, endpoint, buf, *length, 3000, &pr);
  return ret;
}

int pReadData(libusb_device_handle *handle, int endpoint, unsigned char *buf, unsigned int *length)
{
  int ret, pr;
  length = (unsigned int *)length;
  ret = jtag_libusb_bulk_read(handle, endpoint|0x80, buf, *length, 3000, &pr);
  return ret;
}

static struct libusb_context *jtag_libusb_context; /**< Libusb context **/
static struct libusb_device **devs; /**< The usb device list **/

static bool jtag_libusb_match_ids(struct libusb_device_descriptor *dev_desc,
				  const uint16_t vids[], const uint16_t pids[])
{
  for (unsigned i = 0; vids[i]; i++) {
    if (dev_desc->idVendor == vids[i] &&
	dev_desc->idProduct == pids[i]) {
      return true;
    }
  }                                                   
  return false;
}


int device_exists(uint16_t vid, uint16_t pid)
{
  struct libusb_context *context; /**< Libusb context **/
  struct libusb_device **devs; /**< The usb device list **/
  if (libusb_init(&context) < 0)
    return 1;
  int count = libusb_get_device_list(context, &devs);

  struct libusb_device_descriptor desc;

  for (int i = 0; i < count; i++) {
    if (libusb_get_device_descriptor(devs[i], &desc) != 0)
      continue;
    if(desc.idVendor == vid && desc.idProduct == pid) {
      //found
      return 0;
    } 
  }
  if (count >= 0)
    libusb_free_device_list(devs, 1);

  if (context)
    libusb_exit(context);
  return 1;
}

int jtag_libusb_open(const uint16_t vids[], const uint16_t pids[],
		     struct libusb_device_handle **out)
{
  int cnt, idx, err_code;
  int retval = 1;
  struct libusb_device_handle *libusb_handle = NULL;

  if (libusb_init(&jtag_libusb_context) < 0)
    return 1;
  cnt = libusb_get_device_list(jtag_libusb_context, &devs);

  struct libusb_device_descriptor dev_desc;

  for (idx = 0; idx < cnt; idx++) {

    if (libusb_get_device_descriptor(devs[idx], &dev_desc) != 0)
      continue;

    if (!jtag_libusb_match_ids(&dev_desc, vids, pids))
      continue;

    err_code = libusb_open(devs[idx], &libusb_handle);

    if (err_code) {
      fprintf(stderr,"libusb_open() failed with %s",
	      libusb_error_name(err_code));
      continue;
    }
   
    /* Success. */
    *out = libusb_handle;
    retval = 0;
    break;
  }

  if (cnt >= 0)
    libusb_free_device_list(devs, 1);

  if (retval != 0)
    libusb_exit(jtag_libusb_context);

  return retval;
}


int jtag_libusb_set_configuration(struct libusb_device_handle *devh,
				  int configuration)                                          {
  struct libusb_device *udev = libusb_get_device(devh);
  int retval = -99;                                                   
  struct libusb_config_descriptor *config = NULL;
  int current_config = -1;

  retval = libusb_get_configuration(devh, &current_config);
  if (retval != 0)
    return retval;

  retval = libusb_get_config_descriptor(udev, configuration, &config);
  if (retval != 0 || !config)
    return retval;

  /* Only change the configuration if it is not already set to the
     same one. Otherwise this issues a lightweight reset and hangs
     LPC-Link2 with JLink firmware. */
  if (current_config != config->bConfigurationValue)
    retval = libusb_set_configuration(devh, config->bConfigurationValue);

  libusb_free_config_descriptor(config);

  return retval;
}

void jtag_libusb_close(struct libusb_device_handle *dev)
{
  /* Close device */
  libusb_close(dev);

  libusb_exit(jtag_libusb_context);
}

