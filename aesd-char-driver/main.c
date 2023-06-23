/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include <linux/slab.h>
#include <linux/string.h>
#include "aesd_ioctl.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Crankf16"); /** your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev;
    PDEBUG("open");
    
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev;
    
    PDEBUG("end of open");
 
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    
    filp->private_data = NULL;
    
    PDEBUG("end of release");

    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *dev = filp->private_data;
    ssize_t offset = 0; 
    struct aesd_buffer_entry *offset_in;
    ssize_t read_bytes = 0;
    
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    
    if ((mutex_lock_interruptible(&dev->lock)) != 0)
    {
    	PDEBUG(KERN_ERR "Read mutex error");
    }
    
    offset_in = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->circular_buffer, *f_pos, &offset);
    
    if (offset_in == NULL)
    {
    	PDEBUG(KERN_ERR "Read offset error");
    	retval = 0; 
        goto read_end;
    }
    
    read_bytes = ((offset_in->size) - offset);
    if (count < read_bytes)
    {
    	read_bytes = count;
    }
    
    if (copy_to_user(buf, ((offset_in->buffptr) + offset), read_bytes))
    {
    	retval = -EFAULT;
    	goto read_end;
    }
    
    *f_pos += read_bytes;
    retval = read_bytes;
    
    read_end:
    	mutex_unlock(&dev->lock);
    	PDEBUG("end of read");
    
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev = filp->private_data;
    struct aesd_buffer_entry write_buffer;
    char *buffer;
    const char *buffer_new;
    int i, buffer_new_line = 0;
    int temp = 0, size =0;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    
    if ((mutex_lock_interruptible(&dev->lock)) != 0)
    {
    	PDEBUG(KERN_ERR "Write mutex error");
    }
    
    buffer = (char *)kmalloc(count, GFP_KERNEL);
    if (!buffer)
    {
    	PDEBUG("Error in malloc");
        retval = -ENOMEM;
        goto write_end;
    }
    
    if (copy_from_user(buffer, buf, count))
    {
    	PDEBUG("Error in copy");
      	retval = -EFAULT;
  	goto write_end;
    }
    
    for (i = 0 ; i < count; i++)
    {
    	if (buffer[i] == '\n')
    	{
                buffer_new_line = i + 1;
                temp = 1;
        }
    }
    
    if (dev->buffer_length == 0) 
    {
        dev->store_buffer = (char *)kmalloc(count, GFP_KERNEL);
        if (dev->store_buffer == NULL) 
        {
            retval = -ENOMEM;
            goto write_end;
        }
        memcpy(dev->store_buffer, buffer, count);
        dev->buffer_length += count;
    } 
    else 
    {
        if (temp)
            size = buffer_new_line;
        else
            size = count;

        dev->store_buffer = (char *)krealloc(dev->store_buffer, dev->buffer_length + size, GFP_KERNEL);
        if (dev->store_buffer == NULL) 
        {
            retval = -ENOMEM;
            goto write_end;
        }
      
        memcpy(dev->store_buffer + dev->buffer_length, buffer, size);
        dev->buffer_length += size;        
    }

    if (temp) 
    {
        write_buffer.buffptr = dev->store_buffer;
        write_buffer.size = dev->buffer_length;
        buffer_new = aesd_circular_buffer_add_entry(&dev->circular_buffer, &write_buffer);
    
        if (buffer_new != NULL)
            kfree(buffer_new);
        
        dev->buffer_length = 0;
    } 

    retval = count;
    
    write_end:
    	mutex_unlock(&dev->lock);
    	kfree(buffer);
    	PDEBUG("End of write");    
    
    return retval;
}


loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
	loff_t newpos;
	struct aesd_dev *dev;
    	dev = filp->private_data;
    	if (mutex_lock_interruptible(&dev->lock)) {
		return -ERESTARTSYS;
    	}

	switch(whence) {
	  case 0: /* SEEK_SET */
		newpos = off;
		break;

	  case 1: /* SEEK_CUR */
		newpos = filp->f_pos + off;
		break;

	  case 2: /* SEEK_END */
		return -EINVAL;
		break;

	  default: 
		return -EINVAL;
	goto stop;
	}
	
	if (newpos < 0) 
	{
		newpos = -EINVAL;
		goto stop;
	}
	
	filp->f_pos = newpos;
	
stop:
	mutex_unlock(&dev->lock);
	return newpos;
}


long int aesd_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retval = 0;
 
 	if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR) return -ENOTTY;

	switch(cmd) 
	{
	  	case AESDCHAR_IOCSEEKTO:
      		{
        		struct aesd_seekto seekto;
			if (copy_from_user(&seekto, (const void __user *)arg, sizeof(seekto)) != 0) 
			{
            			retval = -EFAULT;
        		} 
        		else 
        		{
            			size_t newpos = aesd_circular_buffer_find_position(
                &aesd_device.circular_buffer, seekto.write_cmd, seekto.write_cmd_offset);
            			filp->f_pos = newpos;
        		}        
      		}
      		break;
        
      		default:
      		return -ENOTTY;
	}
	return retval;
}

struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .llseek =   aesd_llseek,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .unlocked_ioctl = aesd_unlocked_ioctl,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    struct aesd_buffer_entry *buffer_element;
    int temp = 0;
    
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    AESD_CIRCULAR_BUFFER_FOREACH(buffer_element, &aesd_device.circular_buffer, temp)
    {
        if (buffer_element->buffptr != NULL)
        {
            kfree(buffer_element->buffptr);
            buffer_element->size = 0;
        }
    }
    
    mutex_destroy(&aesd_device.lock);
    unregister_chrdev_region(devno, 1);
}


module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
