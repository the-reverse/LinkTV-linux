/************************************************************
 * EFI GUID Partition Table handling
 * Per Intel EFI Specification v1.02
 * http://developer.intel.com/technology/efi/efi.htm
 * efi.[ch] by Matt Domsch <Matt_Domsch@dell.com>
 *   Copyright 2000,2001,2002,2004 Dell Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * TODO:
 *
 * Changelog:
 * Mon Nov 09 2004 Matt Domsch <Matt_Domsch@dell.com>
 * - test for valid PMBR and valid PGPT before ever reading
 *   AGPT, allow override with 'gpt' kernel command line option.
 * - check for first/last_usable_lba outside of size of disk
 *
 * Tue  Mar 26 2002 Matt Domsch <Matt_Domsch@dell.com>
 * - Ported to 2.5.7-pre1 and 2.5.7-dj2
 * - Applied patch to avoid fault in alternate header handling
 * - cleaned up find_valid_gpt
 * - On-disk structure and copy in memory is *always* LE now -
 *   swab fields as needed
 * - remove print_gpt_header()
 * - only use first max_p partition entries, to keep the kernel minor number
 *   and partition numbers tied.
 *
 * Mon  Feb 04 2002 Matt Domsch <Matt_Domsch@dell.com>
 * - Removed __PRIPTR_PREFIX - not being used
 *
 * Mon  Jan 14 2002 Matt Domsch <Matt_Domsch@dell.com>
 * - Ported to 2.5.2-pre11 + library crc32 patch Linus applied
 *
 * Thu Dec 6 2001 Matt Domsch <Matt_Domsch@dell.com>
 * - Added compare_gpts().
 * - moved le_efi_guid_to_cpus() back into this file.  GPT is the only
 *   thing that keeps EFI GUIDs on disk.
 * - Changed gpt structure names and members to be simpler and more Linux-like.
 *
 * Wed Oct 17 2001 Matt Domsch <Matt_Domsch@dell.com>
 * - Removed CONFIG_DEVFS_VOLUMES_UUID code entirely per Martin Wilck
 *
 * Wed Oct 10 2001 Matt Domsch <Matt_Domsch@dell.com>
 * - Changed function comments to DocBook style per Andreas Dilger suggestion.
 *
 * Mon Oct 08 2001 Matt Domsch <Matt_Domsch@dell.com>
 * - Change read_lba() to use the page cache per Al Viro's work.
 * - print u64s properly on all architectures
 * - fixed debug_printk(), now Dprintk()
 *
 * Mon Oct 01 2001 Matt Domsch <Matt_Domsch@dell.com>
 * - Style cleanups
 * - made most functions static
 * - Endianness addition
 * - remove test for second alternate header, as it's not per spec,
 *   and is unnecessary.  There's now a method to read/write the last
 *   sector of an odd-sized disk from user space.  No tools have ever
 *   been released which used this code, so it's effectively dead.
 * - Per Asit Mallick of Intel, added a test for a valid PMBR.
 * - Added kernel command line option 'gpt' to override valid PMBR test.
 *
 * Wed Jun  6 2001 Martin Wilck <Martin.Wilck@Fujitsu-Siemens.com>
 * - added devfs volume UUID support (/dev/volumes/uuids) for
 *   mounting file systems by the partition GUID.
 *
 * Tue Dec  5 2000 Matt Domsch <Matt_Domsch@dell.com>
 * - Moved crc32() to linux/lib, added efi_crc32().
 *
 * Thu Nov 30 2000 Matt Domsch <Matt_Domsch@dell.com>
 * - Replaced Intel's CRC32 function with an equivalent
 *   non-license-restricted version.
 *
 * Wed Oct 25 2000 Matt Domsch <Matt_Domsch@dell.com>
 * - Fixed the last_lba() call to return the proper last block
 *
 * Thu Oct 12 2000 Matt Domsch <Matt_Domsch@dell.com>
 * - Thanks to Andries Brouwer for his debugging assistance.
 * - Code works, detects all the partitions.
 *
 ************************************************************/
#include <linux/config.h>
#include <linux/crc32.h>
#include "check.h"
#include "efi.h"

#undef EFI_DEBUG
#ifdef EFI_DEBUG
#define Dprintk(x...) printk(KERN_DEBUG x)
#else
#define Dprintk(x...)
#endif

/* This allows a kernel command line option 'gpt' to override
 * the test for invalid PMBR.  Not __initdata because reloading
 * the partition tables happens after init too.
 */
static int force_gpt;
static int __init
force_gpt_fn(char *str)
{
	force_gpt = 1;
	return 1;
}
__setup("gpt", force_gpt_fn);


/**
 * efi_crc32() - EFI version of crc32 function
 * @buf: buffer to calculate crc32 of
 * @len - length of buf
 *
 * Description: Returns EFI-style CRC32 value for @buf
 *
 * This function uses the little endian Ethernet polynomial
 * but seeds the function with ~0, and xor's with ~0 at the end.
 * Note, the EFI Specification, v1.02, has a reference to
 * Dr. Dobbs Journal, May 1994 (actually it's in May 1992).
 */
static inline u32
efi_crc32(const void *buf, unsigned long len)
{
	return (crc32(~0L, buf, len) ^ ~0L);
}

/**
 * last_lba(): return number of last logical block of device
 * @bdev: block device
 *
 * Description: Returns last LBA value on success, 0 on error.
 * This is stored (by sd and ide-geometry) in
 *  the part[0] entry for this disk, and is the number of
 *  physical sectors available on the disk.
 */
static u64
last_lba(struct block_device *bdev)
{
	unsigned ssz = bdev_hardsect_size(bdev) / 512;
	unsigned loop = 0;

	if (!bdev || !bdev->bd_inode)
		return 0;

    if(ssz==1)
        loop=0;
    else if(ssz==2)
        loop=1;
    else if(ssz==4)
        loop=2;
    else if(ssz==8)
        loop=3;
    else if(ssz==16)
        loop=4;
    else{
        printk("%s(%d)logic sector size error\n",__func__,__LINE__);
        BUG_ON(1);
    }
	return ((bdev->bd_inode->i_size >> 9) >> loop) - 1ULL;
}

static inline int
pmbr_part_valid(struct partition *part, u64 lastlba)
{
        if (part->sys_ind == EFI_PMBR_OSTYPE_EFI_GPT &&
            le32_to_cpu(part->start_sect) == 1UL)
                return 1;
        return 0;
}

/**
 * is_pmbr_valid(): test Protective MBR for validity
 * @mbr: pointer to a legacy mbr structure
 * @lastlba: last_lba for the whole device
 *
 * Description: Returns 1 if PMBR is valid, 0 otherwise.
 * Validity depends on two things:
 *  1) MSDOS signature is in the last two bytes of the MBR
 *  2) One partition of type 0xEE is found
 */
static int
is_pmbr_valid(legacy_mbr *mbr, u64 lastlba)
{
	int i;
	if (!mbr || le16_to_cpu(mbr->signature) != MSDOS_MBR_SIGNATURE)
                return 0;
	for (i = 0; i < 4; i++)
		if (pmbr_part_valid(&mbr->partition_record[i], lastlba))
                        return 1;
	return 0;
}

/**
 * read_lba(): Read bytes from disk, starting at given LBA
 * @bdev
 * @lba
 * @buffer
 * @size_t
 *
 * Description:  Reads @count bytes from @bdev into @buffer.
 * Returns number of bytes read on success, 0 on error.
 */
static size_t
read_lba(struct block_device *bdev, u64 lba, u8 * buffer, size_t count)
{
	size_t totalreadcount = 0;
	sector_t n = lba * (bdev_hardsect_size(bdev) / 512);

	if (!bdev || !buffer || lba > last_lba(bdev))
                return 0;

	while (count) {
		int copied = 512;
		Sector sect;
		unsigned char *data = read_dev_sector(bdev, n++, &sect);
		if (!data)
			break;
		if (copied > count)
			copied = count;
		memcpy(buffer, data, copied);
		put_dev_sector(sect);
		buffer += copied;
		totalreadcount +=copied;
		count -= copied;
	}
	return totalreadcount;
}

/**
 * alloc_read_gpt_entries(): reads partition entries from disk
 * @bdev
 * @gpt - GPT header
 *
 * Description: Returns ptes on success,  NULL on error.
 * Allocates space for PTEs based on information found in @gpt.
 * Notes: remember to free pte when you're done!
 */
static gpt_entry *
alloc_read_gpt_entries(struct block_device *bdev, gpt_header *gpt)
{
	size_t count;
	gpt_entry *pte;
	if (!bdev || !gpt)
		return NULL;

	count = le32_to_cpu(gpt->num_partition_entries) *
                le32_to_cpu(gpt->sizeof_partition_entry);
	if (!count)
		return NULL;
	pte = kmalloc(count, GFP_KERNEL);
	if (!pte)
		return NULL;
	memset(pte, 0, count);

	if (read_lba(bdev, le64_to_cpu(gpt->partition_entry_lba),
                     (u8 *) pte,
		     count) < count) {
		kfree(pte);
                pte=NULL;
		return NULL;
	}
	return pte;
}

/**
 * alloc_read_gpt_header(): Allocates GPT header, reads into it from disk
 * @bdev
 * @lba is the Logical Block Address of the partition table
 *
 * Description: returns GPT header on success, NULL on error.   Allocates
 * and fills a GPT header starting at @ from @bdev.
 * Note: remember to free gpt when finished with it.
 */
static gpt_header *
alloc_read_gpt_header(struct block_device *bdev, u64 lba)
{
	gpt_header *gpt;
	unsigned ssz = bdev_hardsect_size(bdev);

	if (!bdev)
		return NULL;

	gpt = kmalloc(ssz, GFP_KERNEL);
	if (!gpt)
		return NULL;
	memset(gpt, 0, sizeof (gpt_header));

	if (read_lba(bdev, lba, (u8 *) gpt, ssz) < ssz) {
		kfree(gpt);
                gpt=NULL;
		return NULL;
	}

	return gpt;
}

/**
 * is_gpt_valid() - tests one GPT header and PTEs for validity
 * @bdev
 * @lba is the logical block address of the GPT header to test
 * @gpt is a GPT header ptr, filled on return.
 * @ptes is a PTEs ptr, filled on return.
 *
 * Description: returns 1 if valid,  0 on error.
 * If valid, returns pointers to newly allocated GPT header and PTEs.
 */
static int
is_gpt_valid(struct block_device *bdev, u64 lba,
	     gpt_header **gpt, gpt_entry **ptes)
{
	u32 crc, origcrc;
	u64 lastlba;

	if (!bdev || !gpt || !ptes)
		return 0;
	if (!(*gpt = alloc_read_gpt_header(bdev, lba)))
		return 0;

	/* Check the GUID Partition Table signature */
	if (le64_to_cpu((*gpt)->signature) != GPT_HEADER_SIGNATURE) {
		Dprintk("GUID Partition Table Header signature is wrong:"
			"%lld != %lld\n",
			(unsigned long long)le64_to_cpu((*gpt)->signature),
			(unsigned long long)GPT_HEADER_SIGNATURE);
		goto fail;
	}

	/* Check the GUID Partition Table CRC */
	origcrc = le32_to_cpu((*gpt)->header_crc32);
	(*gpt)->header_crc32 = 0;
	crc = efi_crc32((const unsigned char *) (*gpt), le32_to_cpu((*gpt)->header_size));

	if (crc != origcrc) {
		Dprintk
		    ("GUID Partition Table Header CRC is wrong: %x != %x\n",
		     crc, origcrc);
		goto fail;
	}
	(*gpt)->header_crc32 = cpu_to_le32(origcrc);

	/* Check that the my_lba entry points to the LBA that contains
	 * the GUID Partition Table */
	if (le64_to_cpu((*gpt)->my_lba) != lba) {
		Dprintk("GPT my_lba incorrect: %lld != %lld\n",
			(unsigned long long)le64_to_cpu((*gpt)->my_lba),
			(unsigned long long)lba);
		goto fail;
	}

	/* Check the first_usable_lba and last_usable_lba are
	 * within the disk.
	 */
	lastlba = last_lba(bdev);
	if (le64_to_cpu((*gpt)->first_usable_lba) > lastlba) {
		Dprintk("GPT: first_usable_lba incorrect: %lld > %lld\n",
			(unsigned long long)le64_to_cpu((*gpt)->first_usable_lba),
			(unsigned long long)lastlba);
		goto fail;
	}
	if (le64_to_cpu((*gpt)->last_usable_lba) > lastlba) {
		Dprintk("GPT: last_usable_lba incorrect: %lld > %lld\n",
			(unsigned long long)le64_to_cpu((*gpt)->last_usable_lba),
			(unsigned long long)lastlba);
		goto fail;
	}

	if (!(*ptes = alloc_read_gpt_entries(bdev, *gpt)))
		goto fail;

	/* Check the GUID Partition Entry Array CRC */
	crc = efi_crc32((const unsigned char *) (*ptes),
			le32_to_cpu((*gpt)->num_partition_entries) *
			le32_to_cpu((*gpt)->sizeof_partition_entry));

	if (crc != le32_to_cpu((*gpt)->partition_entry_array_crc32)) {
		Dprintk("GUID Partitition Entry Array CRC check failed.\n");
		goto fail_ptes;
	}

	/* We're done, all's well */
	return 1;

 fail_ptes:
	kfree(*ptes);
	*ptes = NULL;
 fail:
	kfree(*gpt);
	*gpt = NULL;
	return 0;
}

/**
 * is_pte_valid() - tests one PTE for validity
 * @pte is the pte to check
 * @lastlba is last lba of the disk
 *
 * Description: returns 1 if valid,  0 on error.
 */
static inline int
is_pte_valid(const gpt_entry *pte, const u64 lastlba)
{
	if ((!efi_guidcmp(pte->partition_type_guid, NULL_GUID)) ||
	    le64_to_cpu(pte->starting_lba) > lastlba         ||
	    le64_to_cpu(pte->ending_lba)   > lastlba)
		return 0;
	return 1;
}

/**
 * compare_gpts() - Search disk for valid GPT headers and PTEs
 * @pgpt is the primary GPT header
 * @agpt is the alternate GPT header
 * @lastlba is the last LBA number
 * Description: Returns nothing.  Sanity checks pgpt and agpt fields
 * and prints warnings on discrepancies.
 *
 */
static void
compare_gpts(gpt_header *pgpt, gpt_header *agpt, u64 lastlba)
{
	int error_found = 0;
	if (!pgpt || !agpt)
		return;
	if (le64_to_cpu(pgpt->my_lba) != le64_to_cpu(agpt->alternate_lba)) {
		printk(KERN_WARNING
		       "GPT:Primary header LBA != Alt. header alternate_lba\n");
		printk(KERN_WARNING "GPT:%lld != %lld\n",
		       (unsigned long long)le64_to_cpu(pgpt->my_lba),
                       (unsigned long long)le64_to_cpu(agpt->alternate_lba));
		error_found++;
	}
	if (le64_to_cpu(pgpt->alternate_lba) != le64_to_cpu(agpt->my_lba)) {
		printk(KERN_WARNING
		       "GPT:Primary header alternate_lba != Alt. header my_lba\n");
		printk(KERN_WARNING "GPT:%lld != %lld\n",
		       (unsigned long long)le64_to_cpu(pgpt->alternate_lba),
                       (unsigned long long)le64_to_cpu(agpt->my_lba));
		error_found++;
	}
	if (le64_to_cpu(pgpt->first_usable_lba) !=
            le64_to_cpu(agpt->first_usable_lba)) {
		printk(KERN_WARNING "GPT:first_usable_lbas don't match.\n");
		printk(KERN_WARNING "GPT:%lld != %lld\n",
		       (unsigned long long)le64_to_cpu(pgpt->first_usable_lba),
                       (unsigned long long)le64_to_cpu(agpt->first_usable_lba));
		error_found++;
	}
	if (le64_to_cpu(pgpt->last_usable_lba) !=
            le64_to_cpu(agpt->last_usable_lba)) {
		printk(KERN_WARNING "GPT:last_usable_lbas don't match.\n");
		printk(KERN_WARNING "GPT:%lld != %lld\n",
		       (unsigned long long)le64_to_cpu(pgpt->last_usable_lba),
                       (unsigned long long)le64_to_cpu(agpt->last_usable_lba));
		error_found++;
	}
	if (efi_guidcmp(pgpt->disk_guid, agpt->disk_guid)) {
		printk(KERN_WARNING "GPT:disk_guids don't match.\n");
		error_found++;
	}
	if (le32_to_cpu(pgpt->num_partition_entries) !=
            le32_to_cpu(agpt->num_partition_entries)) {
		printk(KERN_WARNING "GPT:num_partition_entries don't match: "
		       "0x%x != 0x%x\n",
		       le32_to_cpu(pgpt->num_partition_entries),
		       le32_to_cpu(agpt->num_partition_entries));
		error_found++;
	}
	if (le32_to_cpu(pgpt->sizeof_partition_entry) !=
            le32_to_cpu(agpt->sizeof_partition_entry)) {
		printk(KERN_WARNING
		       "GPT:sizeof_partition_entry values don't match: "
		       "0x%x != 0x%x\n",
                       le32_to_cpu(pgpt->sizeof_partition_entry),
		       le32_to_cpu(agpt->sizeof_partition_entry));
		error_found++;
	}
	if (le32_to_cpu(pgpt->partition_entry_array_crc32) !=
            le32_to_cpu(agpt->partition_entry_array_crc32)) {
		printk(KERN_WARNING
		       "GPT:partition_entry_array_crc32 values don't match: "
		       "0x%x != 0x%x\n",
                       le32_to_cpu(pgpt->partition_entry_array_crc32),
		       le32_to_cpu(agpt->partition_entry_array_crc32));
		error_found++;
	}
	if (le64_to_cpu(pgpt->alternate_lba) != lastlba) {
		printk(KERN_WARNING
		       "GPT:Primary header thinks Alt. header is not at the end of the disk.\n");
		printk(KERN_WARNING "GPT:%lld != %lld\n",
			(unsigned long long)le64_to_cpu(pgpt->alternate_lba),
			(unsigned long long)lastlba);
		error_found++;
	}

	if (le64_to_cpu(agpt->my_lba) != lastlba) {
		printk(KERN_WARNING
		       "GPT:Alternate GPT header not at the end of the disk.\n");
		printk(KERN_WARNING "GPT:%lld != %lld\n",
			(unsigned long long)le64_to_cpu(agpt->my_lba),
			(unsigned long long)lastlba);
		error_found++;
	}

	if (error_found)
		printk(KERN_WARNING
		       "GPT: Use GNU Parted to correct GPT errors.\n");
	return;
}

/**
 * find_valid_gpt() - Search disk for valid GPT headers and PTEs
 * @bdev
 * @gpt is a GPT header ptr, filled on return.
 * @ptes is a PTEs ptr, filled on return.
 * Description: Returns 1 if valid, 0 on error.
 * If valid, returns pointers to newly allocated GPT header and PTEs.
 * Validity depends on PMBR being valid (or being overridden by the
 * 'gpt' kernel command line option) and finding either the Primary
 * GPT header and PTEs valid, or the Alternate GPT header and PTEs
 * valid.  If the Primary GPT header is not valid, the Alternate GPT header
 * is not checked unless the 'gpt' kernel command line option is passed.
 * This protects against devices which misreport their size, and forces
 * the user to decide to use the Alternate GPT.
 */
static int
find_valid_gpt(struct block_device *bdev, gpt_header **gpt, gpt_entry **ptes)
{
	int good_pgpt = 0, good_agpt = 0, good_pmbr = 0;
	gpt_header *pgpt = NULL, *agpt = NULL;
	gpt_entry *pptes = NULL, *aptes = NULL;
	legacy_mbr *legacymbr = NULL;
	u64 lastlba;
	if (!bdev || !gpt || !ptes)
		return 0;

	lastlba = last_lba(bdev);
    if (!force_gpt) {
        /* This will be added to the EFI Spec. per Intel after v1.02. */
        legacymbr = kmalloc(sizeof (*legacymbr), GFP_KERNEL);
        if (legacymbr) {
            memset(legacymbr, 0, sizeof (*legacymbr));
            read_lba(bdev, 0, (u8 *) legacymbr,
                     sizeof (*legacymbr));
            good_pmbr = is_pmbr_valid(legacymbr, lastlba);
            kfree(legacymbr);
            legacymbr=NULL;
        }
                if (!good_pmbr)
            goto fail;
        }

	good_pgpt = is_gpt_valid(bdev, GPT_PRIMARY_PARTITION_TABLE_LBA,
				 &pgpt, &pptes);
        if (good_pgpt)
	    good_agpt = is_gpt_valid(bdev,
				 le64_to_cpu(pgpt->alternate_lba),
				 &agpt, &aptes);
        if (!good_agpt && force_gpt)
            good_agpt = is_gpt_valid(bdev, lastlba,
                                     &agpt, &aptes);

        /* The obviously unsuccessful case */
        if (!good_pgpt && !good_agpt)
                goto fail;

    compare_gpts(pgpt, agpt, lastlba);

        /* The good cases */
        if (good_pgpt) {
                *gpt  = pgpt;
                *ptes = pptes;
                kfree(agpt);
                kfree(aptes);
                if (!good_agpt) {
                        printk(KERN_WARNING
			       "Alternate GPT is invalid, "
                               "using primary GPT.\n");
                }
                return 1;
        }
        else if (good_agpt) {
                *gpt  = agpt;
                *ptes = aptes;
                kfree(pgpt);
                kfree(pptes);
                printk(KERN_WARNING
                       "Primary GPT is invalid, using alternate GPT.\n");
                return 1;
        }

 fail:
        kfree(pgpt);
        kfree(agpt);
        kfree(pptes);
        kfree(aptes);
        *gpt = NULL;
        *ptes = NULL;
        return 0;
}

/**
 * efi_partition(struct parsed_partitions *state, struct block_device *bdev)
 * @state
 * @bdev
 *
 * Description: called from check.c, if the disk contains GPT
 * partitions, sets up partition entries in the kernel.
 *
 * If the first block on the disk is a legacy MBR,
 * it will get handled by msdos_partition().
 * If it's a Protective MBR, we'll handle it here.
 *
 * We do not create a Linux partition for GPT, but
 * only for the actual data partitions.
 * Returns:
 * -1 if unable to read the partition table
 *  0 if this isn't our partition table
 *  1 if successful
 *
 */
int
efi_partition(struct parsed_partitions *state, struct block_device *bdev)
{
	gpt_header *gpt = NULL;
	gpt_entry *ptes = NULL;
	u32 i;
	unsigned ssz = bdev_hardsect_size(bdev) / 512;

	if (!find_valid_gpt(bdev, &gpt, &ptes) || !gpt || !ptes) {
		kfree(gpt);
		kfree(ptes);
		return 0;
	}

	Dprintk("GUID Partition Table is valid!  Yea!\n");

	for (i = 0; i < le32_to_cpu(gpt->num_partition_entries) && i < state->limit-1; i++) {
		u64 start = le64_to_cpu(ptes[i].starting_lba);
		u64 size = le64_to_cpu(ptes[i].ending_lba) -
			   le64_to_cpu(ptes[i].starting_lba) + 1ULL;

		if (!is_pte_valid(&ptes[i], last_lba(bdev)))
			continue;

		put_partition(state, i+1, start * ssz, size * ssz);

		/* If this is a RAID volume, tell md */
		if (!efi_guidcmp(ptes[i].partition_type_guid,
				 PARTITION_LINUX_RAID_GUID))
			state->parts[i+1].flags = 1;

		/* If this is a EFI System partition, tell hotplug */
		if (!efi_guidcmp(ptes[i].partition_type_guid,
				 PARTITION_SYSTEM_GUID))
			state->parts[i+1].is_efi_system_partition = 1;
	}
	kfree(ptes);
	kfree(gpt);
	printk("\n");
	return 1;
}
