/*
 * khashmap.c - Generic key/value hash map implementation
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
#include <linux/list.h>
#include <linux/hash.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/vmalloc.h>

#include "khashmap.h"

#define khashmap_hashfn(__hash, __id) \
		hash_long((unsigned long)__id, __hash->bits)

static struct kmem_cache *hlist_cachep __read_mostly;

bool khashmap_empty(const struct khashmap *hlist)
{
	struct khashmap_item *item;
	struct hlist_node *node;
	int i;

	if (likely(hlist))
		khashmap_for_each_entry(hlist, i, node, item)
			return false;
	return true;

}
EXPORT_SYMBOL(khashmap_empty);

static struct khashmap_item *
khashmap_find_item(const struct khashmap *hlist, u64 key)
{
        struct hlist_node *node;
        struct khashmap_item *item;

        hlist_for_each_entry(item, node,
			     &hlist->hash[khashmap_hashfn(hlist, key)], hlist)
                if (item->key == key)
                        return item;
	return NULL;
}

void *khashmap_find(const struct khashmap *hlist, u64 key)
{
	struct khashmap_item *item = khashmap_find_item(hlist, key);

        return item ? item->val : NULL;
}
EXPORT_SYMBOL(khashmap_find);

int khashmap_add(struct khashmap *hlist, u64 key, void *val, gfp_t flags)
{
	struct khashmap_item *item = khashmap_find_item(hlist, key);

	if (item) {
		item->val = val;
		return 0;
	}

	item = kmem_cache_zalloc(hlist_cachep, flags);
	if (unlikely(!item))
		return -ENOMEM;
	item->key = key;
	item->val = val;
	hlist_add_head(&item->hlist, &hlist->hash[khashmap_hashfn(hlist, key)]);

	return 0;
}
EXPORT_SYMBOL(khashmap_add);

void khashmap_del(struct khashmap *hlist, u64 key)
{
	struct khashmap_item *item;

	item = khashmap_find_item(hlist, key);
	if (!item)
		return;
	hlist_del(&item->hlist);
	kmem_cache_free(hlist_cachep, item);
}
EXPORT_SYMBOL(khashmap_del);

static int khashmap_alloc(struct khashmap *hlist)
{
	size_t size = khashmap_size_in_bytes(hlist);
	int i;

	if (size < PAGE_SIZE)
		hlist->hash = kmalloc(size, GFP_KERNEL);
	else
		hlist->hash = vmalloc(size);
	if (unlikely(!hlist->hash))
		return -ENOMEM;
	for (i = 0; i < khashmap_size(hlist); i++)
		INIT_HLIST_HEAD(&hlist->hash[i]);
	return 0;
}

static void khashmap_free(struct khashmap *hlist)
{
	size_t size = khashmap_size_in_bytes(hlist);

	if (size < PAGE_SIZE)
		kfree(hlist->hash);
	else
		vfree(hlist->hash);
}

void khashmap_destroy(struct khashmap *hlist)
{
	struct khashmap_item *item;
	struct hlist_node *node, *p;
	int i;

	if (unlikely(!hlist))
		return;
	khashmap_for_each_entry_safe(hlist, i, node, p, item) {
		hlist_del(&item->hlist);
		kmem_cache_free(hlist_cachep, item);
	}
	khashmap_free(hlist);
}
EXPORT_SYMBOL(khashmap_destroy);

int khashmap_init(struct khashmap *hlist)
{
	return khashmap_alloc(hlist);
}
EXPORT_SYMBOL(khashmap_init);

static int __init khashmap_module_init(void)
{
	hlist_cachep = kmem_cache_create("khashmap_cache",
					     sizeof(struct khashmap_item),
					     0, 0, NULL);
        if (unlikely(!hlist_cachep)) {
                printk(KERN_ERR "hlist: failed to create slab cache\n");
                return -ENOMEM;
        }
        return 0;
}

static void __exit khashmap_module_exit(void)
{
	kmem_cache_destroy(hlist_cachep);
}

module_init(khashmap_module_init);
module_exit(khashmap_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrea Righi <andrea@betterlinux.com>");
MODULE_DESCRIPTION("Generic key/value hash map");
