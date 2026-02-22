#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/stat.h>
#include <linux/fb.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>

struct msp3520fb_hw_info{
  u32 w;
  u32 h;
  u32 bpp;
  u32 fps;
};

struct msp3520fb_hw_info msp3520fb_display = {
  .w = 128,
  .h = 64,
  .bpp = 24,
  .fps = 30
};

int msp3520fb_probe(struct platform_device *dev);
void msp3520fb_remove(struct platform_device *dev);

static struct platform_driver msp3520fb_driver = {
  .probe = msp3520fb_probe,
  .remove = msp3520fb_remove,
  .driver = {
    .name = "msp3520fb",
  },
};

static struct platform_device *msp3520fb_platform_dev;

static ssize_t dummy_fb_read(struct fb_info *info, char __user *buf,
			   size_t count, loff_t *ppos)
{
    pr_info("%s\n", __func__);
    printk("read something\n");
    return fb_sys_read(info, buf, count, ppos);
}

static ssize_t dummy_fb_write(struct fb_info *info, const char __user *buf,
			    size_t count, loff_t *ppos)
{
    ssize_t ret = 0;
    pr_info("%s: count=%zd, ppos=%llu\n", __func__,  count, *ppos);
    ret = fb_sys_write(info, buf, count, ppos);
    schedule_delayed_work(&info->deferred_work, info->fbdefio->delay);
    return ret;
}


static void dummy_fb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
    pr_info("%s\n", __func__);
    sys_fillrect(info, rect);
}

static void dummy_fb_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
    pr_info("%s\n", __func__);
    sys_copyarea(info, area);
}

static void dummy_fb_imageblit(struct fb_info *info, const struct fb_image *image)
{
    pr_info("%s\n", __func__);
    sys_imageblit(info, image);
}


static int dummy_mmap(struct fb_info *info, struct vm_area_struct *vma){
  printk("mmaped");
  return fb_deferred_io_mmap(info, vma);
}

static void msp3520fb_deferred_io(struct fb_info *info,
					     struct list_head *pagelist){
  printk("defio\n");

  for(u8 *a = (u8*)info->screen_buffer; a < info->screen_buffer+info->fix.smem_len; a++){
    //if(*a)
      //pr_info("%02X ", *a);
  }
  printk("\n");

  struct fb_deferred_io_pageref *pageref;
    uint y_cur, y_end;

    list_for_each_entry(pageref, pagelist, list) {
        y_cur = pageref->offset / info->fix.line_length;
        y_end = (pageref->offset + PAGE_SIZE) / info->fix.line_length;

        loff_t b = (y_end - y_cur) * info->fix.line_length;
        
          //dummy_fb_read(info, , b, &written);
        

        printk("pageref offset %ld; page size: %ld \n", pageref->offset, PAGE_SIZE);
        printk("smem start %ld; smem size: %ld \n", info->fix.smem_start, info->fix.smem_len);
        printk("y_end: %ld; b: %ld\n", y_end, b);
        break;
    } 
}

/* from pxafb.c */
static unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
    chan &= 0xffff;
    chan >>= 16 - bf->length;
    return chan << bf->offset;
}

static int dummy_setcolreg(unsigned int regno, unsigned int red,
                                unsigned int green, unsigned int blue,
                                unsigned int transp, struct fb_info *info)
{
    unsigned int val;
    int ret = 1;

    //printk("%s\n", __func__);
    //pr_info("%s(regno=%u, red=0x%X, green=0x%X, blue=0x%X, trans=0x%X)\n",
    //       __func__, regno, red, green, blue, transp);

    if (regno >= 256)   /* no. of hw registers */
        return 1;

    switch (info->fix.visual) {
      case FB_VISUAL_PSEUDOCOLOR:
        if (regno < 16) {
            val  = chan_to_field(red, &info->var.red);
            val |= chan_to_field(green, &info->var.green);
            val |= chan_to_field(blue, &info->var.blue);

            ((u32 *)(info->pseudo_palette))[regno] = val;
            ret = 0;
        }
      break;
    }

    return ret;
}

static int dummy_checkvar(struct fb_var_screeninfo *var, struct fb_info *info){
  printk("%s\n", __func__);
  printk("%d %d %d\n", var->red.length, var->green.length, var->blue.length);

  return 0;
}

static int dummy_checkcmap(struct fb_cmap *cmap, struct fb_info *info){
  printk("%s\n", __func__);
  //printk("%hd %hd %hd\n", cmap->red, cmap->green, cmap->blue);
  //return fb_set_cmap(cmap, info);
  return 0;
}

static int dummy_set_par(struct fb_info *info){
  printk("%s\n", __func__);
  return 0;
}


int msp3520fb_probe(struct platform_device *dev){
  u32 width = msp3520fb_display.w;
  u32 height = msp3520fb_display.h;
  u32 bits_per_pixel = msp3520fb_display.bpp;

  struct fb_info *info;
  struct device *device = &dev->dev;

  u64 vmem_size = (width * height * bits_per_pixel) / 8;
  u8 *vmem = vmalloc(vmem_size);

  if(!vmem){
    printk("msp3520fb: failed to allocate buffer memory\n");
    return -ENOMEM;
  }

  info = framebuffer_alloc(0, device);
  if(!info){
    printk("msp3520fb: failed to allocate framebuffer info\n");
    goto screenbuffer_cleanup;
  }

  info->screen_buffer = vmem;
  info->screen_base = vmem;

  snprintf(info->fix.id, sizeof(info->fix.id), "%s", "msp3520fb");
  info->fix.type = FB_TYPE_PACKED_PIXELS;
  info->fix.visual = FB_VISUAL_PSEUDOCOLOR;
  info->fix.xpanstep = 0;
  info->fix.ypanstep = 0;
  info->fix.ywrapstep = 0;
  info->fix.line_length = width * bits_per_pixel / 8;
  info->fix.accel = FB_ACCEL_NONE;
  info->fix.smem_start = 0;
  info->fix.smem_len = vmem_size;

  info->var.xres = width;
  info->var.yres = height;
  info->var.xres_virtual = width;
  info->var.yres_virtual = height;
  info->var.bits_per_pixel = bits_per_pixel;
  info->var.nonstd = 0;
  info->var.grayscale = 0;

  info->var.red.offset    = 0;
  info->var.red.length    = 8;
  info->var.green.offset  = 8;
  info->var.green.length  = 8;
  info->var.blue.offset   = 16;
  info->var.blue.length   = 8;
  info->var.transp.offset = 0;
  info->var.transp.length = 0;


  //if (fb_alloc_cmap(&info->cmap, 256, 0))
	//  goto framebuffer_cleanup;

  struct fb_ops *fbops = kzalloc(sizeof(struct fb_ops), GFP_KERNEL);
  if(!fbops) {
    goto framebuffer_cleanup;
  }

  fbops->owner = THIS_MODULE;
  fbops->fb_read = dummy_fb_read;
  fbops->fb_write = dummy_fb_write;
  fbops->fb_mmap = dummy_mmap;
  fbops->fb_fillrect = dummy_fb_fillrect;
  fbops->fb_copyarea = dummy_fb_copyarea;
  fbops->fb_imageblit = dummy_fb_imageblit;
  fbops->fb_setcolreg = dummy_setcolreg;
  fbops->fb_check_var = dummy_checkvar;
  fbops->fb_setcmap = dummy_checkcmap;
  fbops->fb_set_par = dummy_set_par;

  struct fb_deferred_io *fbdefio = kzalloc(sizeof(*fbdefio), GFP_KERNEL);
  if(!fbdefio){
    goto framebuffer_cleanup;
  }

  fbdefio->deferred_io = msp3520fb_deferred_io;
  fbdefio->delay = 0;
  info->fbdefio = fbdefio;

  info->fbops = fbops;
  info->flags = FBINFO_VIRTFB;
  
  fb_deferred_io_init(info);

  int ret = register_framebuffer(info);
  if(ret < 0){
    printk("msp3520fb: failed to register framebuffer info\n");
    goto framebuffer_cleanup;
  }  

  platform_set_drvdata(msp3520fb_platform_dev, info);

  printk("msp3520fb: probed platform device\n");
  return 0;

framebuffer_cleanup:
  if(info->fbops){
    kfree(info->fbops);
  }
  unregister_framebuffer(info);
  //fb_dealloc_cmap(&info->cmap);
  framebuffer_release(info);
screenbuffer_cleanup:
  kfree(vmem);

return -ENOMEM;
}

static int __init msp3520fb_init(void)
{
  pr_info("%s\n", __func__);

  msp3520fb_platform_dev = kzalloc(sizeof(*msp3520fb_platform_dev), GFP_KERNEL);
  if(!msp3520fb_platform_dev){
    printk("error allocating platform device\n");
    return -ENOMEM;
  }

  int ret;
  ret = platform_driver_register(&msp3520fb_driver);
  if(ret){ //error
    printk("failed to register platform driver\n");
    platform_driver_unregister(&msp3520fb_driver);
    return ret;
  }

  msp3520fb_platform_dev = platform_device_register_simple("msp3520fb", 0, NULL, 0);
  if(IS_ERR(msp3520fb_platform_dev)){
    printk("failed to register platform device\n");
    platform_device_unregister(msp3520fb_platform_dev);
    return 1;
  }
  pr_info("initialized msp3520fb framebuffer driver\n");
  return 0;
}

void msp3520fb_remove(struct platform_device *dev){
  pr_info("%s\n", __func__);

  struct fb_info *info = platform_get_drvdata(msp3520fb_platform_dev);

  if (info) {
      fb_deferred_io_cleanup(info);
      unregister_framebuffer(info);
      kvfree(info->screen_buffer);
      fb_dealloc_cmap(&info->cmap);
      framebuffer_release(info);
  }
}

static void __exit msp3520fb_exit(void)
{
  pr_info("%s\n", __func__);

  platform_driver_unregister(&msp3520fb_driver);
  if(msp3520fb_platform_dev)
    platform_device_unregister(msp3520fb_platform_dev);
}



MODULE_DESCRIPTION("msp3520fb");
MODULE_LICENSE("GPL");

module_init(msp3520fb_init);
module_exit(msp3520fb_exit);