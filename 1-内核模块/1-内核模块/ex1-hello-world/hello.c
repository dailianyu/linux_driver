/*
 * hello.c
 *
 * Simple hello world 2.6 driver module
 *
 * Copyright (C) 2010 www.embedhq.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE ("GPL");

int init_module (void)
{
	printk (KERN_INFO "Hello world\n");
	return 0;
}

void cleanup_module (void)
{
	printk (KERN_INFO "Goodbye world\n");
}
