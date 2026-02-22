#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

int main(void){
    int fd = open("/dev/fb1", O_RDWR);
    if(fd == -1){
        perror("open");
        return -1;
    }

    struct fb_var_screeninfo varinfo;

    int res = ioctl(fd, FBIOGET_VSCREENINFO, &varinfo);
    if (res == -1) {
        perror("ioctl");
        close(fd);
        return 1;
    }

    uint32_t line_len = (varinfo.xres * varinfo.bits_per_pixel) / 8;
    
    printf("screen_w: %u screen_y: %u\n", varinfo.xres, varinfo.yres);
    printf("bps: %u line_len %u\n", varinfo.bits_per_pixel, line_len);
    

    char *buff;
    buff = mmap(0, line_len * varinfo.yres, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(buff == -1){
        perror("mmap");
        return -1;
    }

    buff[64*7] = 1;
    buff[64*7+1] = 255;

    memset(buff+64, 0xFF, 128);

    munmap(buff, line_len * varinfo.yres);

    close(fd);
    return 0;
}