/*
 * khashmap example
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 *
 * Copyright (C) 2013 Andrea Righi <andrea@betterlinux.com>
 */

#include <linux/init.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/cryptohash.h>
#include <linux/percpu-defs.h>

#include "khashmap.h"

static DEFINE_KHASHMAP(hash);
static DEFINE_MUTEX(hash_lock);

static const char procfs_name[] = "hashmap";

static struct proc_dir_entry *procfs_file;

static unsigned int get_random_int(void)
{
	__u32 hash, random;
	unsigned int ret;

	hash = get_cycles();
	md5_transform(&hash, &random);
	ret = hash;

	return ret;
}

static void hash_dump(struct seq_file *m)
{
	struct khashmap_item *item;
	struct hlist_node *node;
	int i;

	khashmap_for_each_entry(&hash, i, node, item)
		seq_printf(m, "  %llu -> %lu\n",
			   item->key, (unsigned long)item->val);
}

static int procfs_read(struct seq_file *m, void *v)
{
	unsigned long key = get_random_int() % 1000;
	unsigned long val;

	seq_puts(m, "hash dump:\n");

	mutex_lock(&hash_lock);
	hash_dump(m);
	val = (unsigned long)khashmap_find(&hash, key);
	mutex_unlock(&hash_lock);

	if (!val)
		seq_printf(m, "key %lu not found\n", key);
	else
		seq_printf(m, "key=%lu value=%lu\n", key, (unsigned long)val);
	return 0;
}

static int procfs_open(struct inode *inode, struct file *file)
{
	int ret;

	mutex_lock(&hash_lock);
	ret = single_open(file, procfs_read, NULL);
	mutex_unlock(&hash_lock);

	return ret;
}

static int procfs_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations procfs_fops = {
	.open		= procfs_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= procfs_release,
};

static int hash_init(void)
{
	int i;

	khashmap_init(&hash);

	/* Fill the hash map with random values */
	for (i = 0; i < 1000; i++) {
		unsigned long key = get_random_int() % 1000;
		unsigned long val = get_random_int() % 1000 + 1;
		int ret;

		ret = khashmap_add(&hash, key, (void *)val, GFP_KERNEL);
		if (unlikely(ret < 0)) {
			khashmap_destroy(&hash);
			return ret;
		}
	}
	return 0;
}

static int __init khashmap_example_init(void)
{
	int ret;

	procfs_file = proc_create(procfs_name, 0666, NULL, &procfs_fops);
	if (unlikely(!procfs_file))
		return -ENOMEM;
	ret = hash_init();
	if (unlikely(ret < 0))
		remove_proc_entry(procfs_name, NULL);
	return ret;
}

static void __exit khashmap_example_exit(void)
{
	khashmap_destroy(&hash);
	remove_proc_entry(procfs_name, NULL);
}

module_init(khashmap_example_init);
module_exit(khashmap_example_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("key/value hash map example");
MODULE_AUTHOR("Andrea Righi <andrea@betterlinux.com>");
