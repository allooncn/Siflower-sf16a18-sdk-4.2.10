/*
 * OF helpers for network devices.
 *
 * This file is released under the GPLv2
 *
 * Initially copied out of arch/powerpc/kernel/prom_parse.c
 */
#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/of_net.h>
#include <linux/phy.h>
#include <linux/export.h>
#include <linux/mtd/mtd.h>

/**
 * of_get_phy_mode - Get phy mode for given device_node
 * @np:	Pointer to the given device_node
 *
 * The function gets phy interface string from property 'phy-mode' or
 * 'phy-connection-type', and return its index in phy_modes table, or errno in
 * error case.
 */
int of_get_phy_mode(struct device_node *np)
{
	const char *pm;
	int err, i;

	err = of_property_read_string(np, "phy-mode", &pm);
	if (err < 0)
		err = of_property_read_string(np, "phy-connection-type", &pm);
	if (err < 0)
		return err;

	for (i = 0; i < PHY_INTERFACE_MODE_MAX; i++)
		if (!strcasecmp(pm, phy_modes(i)))
			return i;

	return -ENODEV;
}
EXPORT_SYMBOL_GPL(of_get_phy_mode);

/**
 * Search the device tree for the best MAC address to use.  'mac-address' is
 * checked first, because that is supposed to contain to "most recent" MAC
 * address. If that isn't set, then 'local-mac-address' is checked next,
 * because that is the default address.  If that isn't set, then the obsolete
 * 'address' is checked, just in case we're using an old device tree.
 *
 * Note that the 'address' property is supposed to contain a virtual address of
 * the register set, but some DTS files have redefined that property to be the
 * MAC address.
 *
 * All-zero MAC addresses are rejected, because those could be properties that
 * exist in the device tree, but were not set by U-Boot.  For example, the
 * DTS could define 'mac-address' and 'local-mac-address', with zero MAC
 * addresses.  Some older U-Boots only initialized 'local-mac-address'.  In
 * this case, the real MAC is in 'local-mac-address', and 'mac-address' exists
 * but is all zeros.
*/
const void *of_get_mac_address(struct device_node *np)
{
	struct property *pp;

	pp = of_find_property(np, "mac-address", NULL);
	if (pp && (pp->length == 6) && is_valid_ether_addr(pp->value))
		return pp->value;

	pp = of_find_property(np, "local-mac-address", NULL);
	if (pp && (pp->length == 6) && is_valid_ether_addr(pp->value))
		return pp->value;

	pp = of_find_property(np, "address", NULL);
	if (pp && (pp->length == 6) && is_valid_ether_addr(pp->value))
		return pp->value;

	return NULL;
}
EXPORT_SYMBOL(of_get_mac_address);

int of_get_mac_address_mtd(struct device_node *np, void *mac)
{
	struct device_node *mtd_np = NULL;
	size_t retlen;
	int size, ret;
	struct mtd_info *mtd;
	const char *part;
	const __be32 *list;
	phandle phandle;
	u_char * pmac = (u_char *)mac;
	u32 mac_inc;

	list = of_get_property(np, "mtd-mac-address", &size);
	if (!list || (size != (2 * sizeof(*list))))
		return -ENOENT;

	phandle = be32_to_cpup(list++);
	if (phandle)
		mtd_np = of_find_node_by_phandle(phandle);

	if (!mtd_np)
		return -ENOENT;

	part = of_get_property(mtd_np, "label", NULL);
	if (!part)
		part = mtd_np->name;

	mtd = get_mtd_device_nm(part);
	if (IS_ERR(mtd))
		return PTR_ERR(mtd);

	ret = mtd_read(mtd, be32_to_cpup(list), 6, &retlen, (u_char *) mac);
	put_mtd_device(mtd);

	if (!of_property_read_u32(np, "mtd-mac-address-increment", &mac_inc))
		pmac[5] += mac_inc;

	return ret;
}
EXPORT_SYMBOL_GPL(of_get_mac_address_mtd);
