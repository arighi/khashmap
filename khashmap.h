/*
 * khashmap.h - Generic key/value hash map implementation
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

#ifndef _KHASHMAP_H_
#define _KHASHMAP_H

#include <linux/types.h>
#include <linux/list.h>

/**
 * struct khashmap - define a key-value hash map
 * @bits: amount of bits used to size the hash array (size is 2^bits)
 * @hash: the hash list head
 *
 * NOTE: more bits can be used to decrease the probability of key conflicts,
 * however this would make the hash bigger using more memory, but, more
 * important, a full scan of the hash will trash the CPUs caches even more.
 *
 * The suggested value for bits is 9.
 */
struct khashmap {
	size_t bits;
	struct hlist_head *hash;
};

/**
 * struct khashmap_item - define a key-value item
 * @key: the item's key
 * @value: the item's value
 * @hlist: used to link the item to a khashmap
 */
struct khashmap_item {
	u64 key;
	void *val;
	struct hlist_node hlist;
};

/**
 * DECLARE_KHASHMAP - declare a khashmap object
 * @__name: name of the declared khashmap object.
 */
#define DECLARE_KHASHMAP(__name) struct khashmap __name

/**
 * DEFINE_KHASHMAP_BITS - instantiate a khashmap object
 * @__name: name of the declared khashmap object.
 * @__bits: amount of bits used to size the hash array.
 */
#define DEFINE_KHASHMAP_BITS(__name, __bits)		\
		struct khashmap __name = {		\
			.bits = (__bits),		\
			.hash = NULL,			\
		}

/**
 * DEFINE_KHASHMAP - instantiate a khashmap object using the default array size
 * @__name: name of the declared khashmap object.
 */
#define DEFINE_KHASHMAP(__name)				\
		DEFINE_KHASHMAP_BITS(__name, 9)

/**
 * khashmap_for_each_entry - iterate over all the khashmap items
 * @__hash: the khashmap object.
 * @__pos: an integer used to iterate over the hash array.
 * @__node: a hlist_node object used to iterate over the hash list items.
 * @__item: a khashmap_item object representing each item in the hash list.
 */
#define khashmap_for_each_entry(__hash,	__pos, __node, __item)		\
        for (__pos = 0; __pos < khashmap_size(__hash); __pos++)		\
                hlist_for_each_entry(__item, __node,			\
				     &(__hash)->hash[(__pos)], hlist)
/**
 * khashmap_for_each_entry - iterate over all the khashmap items (safe against
 * removals)
 * @__hash: the khashmap object.
 * @__pos: an integer used to iterate over the hash array.
 * @__node: a hlist_node object used to iterate over the hash list items.
 * @__p: another hlist_node object used as a temporary storage.
 * @__item: a khashmap_item object representing each item in the hash list.
 */
#define khashmap_for_each_entry_safe(__hash,				\
				     __pos, __node, __p, __item)	\
	for (__pos = 0; __pos < khashmap_size(__hash); __pos++)		\
		hlist_for_each_entry_safe(__item, __p,			\
				  __node, &(__hash)->hash[(__pos)], hlist)

/**
 * khashmap_size - return the amount of elements in the hash array
 *
 * @__hlist: the khashmap object.
 *
 * NOTE: this function returns the amount of hlist_head elements in the hash
 * list array.
 *
 * Use khashmap_size_in_bytes() to get the size of the array in bytes (see
 * below).
 */
static inline size_t khashmap_size(const struct khashmap *hlist)
{
	return 1UL << hlist->bits;
}

/**
 * khashmap_size - return the size of the hash array in bytes
 *
 * @__hlist: the khashmap object.
 */
static inline size_t khashmap_size_in_bytes(const struct khashmap *hlist)
{
	return khashmap_size(hlist) * sizeof(struct hlist_head);
}

/**
 * khashmap_empty - return true if the hash list is empty, false otherwise
 *
 * @hlist: the khashmap object.
 */
bool khashmap_empty(const struct khashmap *hlist);

/**
 * khashmap_find - find an element in the hash list.
 *
 * @hlist: the khashmap object.
 * @key: the key to find.
 *
 * Returns a value associated to key, NULL otherwise.
 */
void *khashmap_find(const struct khashmap *hlist, u64 key);

/**
 * khashmap_add - add a key/value element to the hash map
 *
 * @hlist: the khashmap object.
 * @key: the key of the element.
 * @val: the value of the element.
 * @flags: flags used to allocate the new item.
 *
 * Returns 0 in case of success, a negative value otherwise.
 *
 * NOTE: if the element is already present in the list, the new value will
 * replace the old one. Otherwise a new khashmap_item will be allocated to
 * store the key/value pair into the hash.
 */
int khashmap_add(struct khashmap *hlist, u64 key, void *val, gfp_t flags);

/**
 * khashmap_del - remove a key/value element from the hash map
 *
 * @hlist: the khashmap object.
 * @key: the key of the element to remove.
 */
void khashmap_del(struct khashmap *hlist, u64 key);

/**
 * khashmap_init - initialize a khashmap object
 *
 * @hlist: the khashmap object.
 *
 * Returns 0 in case of success, a negative value otherwise.
 */
int khashmap_init(struct khashmap *hlist);

/**
 * khashmap_destroy - destroy a khashmap object
 *
 * @hlist: the khashmap object.
 */
void khashmap_destroy(struct khashmap *hlist);

#endif /* _KHASHMAP_H */
