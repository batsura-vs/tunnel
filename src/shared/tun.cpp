#include "tun.h"

#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>

// ЗАИМСТВОВАННЫЙ КОД https://docs.kernel.org/networking/tuntap.html
int tun_alloc(char* dev, const char* device_path) {
  ifreq ifr{};
  int fd;

  if ((fd = open(device_path, O_RDWR)) < 0) return fd;

  memset(&ifr, 0, sizeof(ifr));

  /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
   *        IFF_TAP   - TAP device
   *
   *        IFF_NO_PI - Do not provide packet information
   */
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

  if (*dev) {
    std::strncpy(ifr.ifr_name, dev, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
  }

  if (int err; (err = ioctl(fd, TUNSETIFF, static_cast<void*>(&ifr))) < 0) {
    close(fd);
    return err;
  }
  strcpy(dev, ifr.ifr_name);
  return fd;
}
// КОНЕЦ ЗАИМСТВОВАННОГО КОДА https://docs.kernel.org/networking/tuntap.html
