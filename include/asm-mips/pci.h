/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */
#ifndef _ASM_PCI_H
#define _ASM_PCI_H

#include <linux/config.h>
#include <linux/mm.h>

#ifdef __KERNEL__

/* Can be used to override the logic in pci_scan_bus for skipping
   already-configured bus numbers - to be used for buggy BIOSes
   or architectures with incomplete PCI setup by the loader */

#ifdef CONFIG_PCI
extern unsigned int pcibios_assign_all_busses(void);
#else
#define pcibios_assign_all_busses()	0
#endif

#define PCIBIOS_MIN_IO		0x1000
#define PCIBIOS_MIN_MEM		0x10000000

static inline void pcibios_set_master(struct pci_dev *dev)
{
	/* No special bus mastering setup handling */
}

static inline void pcibios_penalize_isa_irq(int irq)
{
	/* We don't do dynamic PCI IRQ allocation */
}

/*
 * Dynamic DMA mapping stuff.
 * MIPS has everything mapped statically.
 */

#include <linux/types.h>
#include <linux/slab.h>
#include <asm/scatterlist.h>
#include <linux/string.h>
#include <asm/io.h>

#if defined(CONFIG_DDB5074) || defined(CONFIG_DDB5476)
#undef PCIBIOS_MIN_IO
#undef PCIBIOS_MIN_MEM
#define PCIBIOS_MIN_IO		0x0100000
#define PCIBIOS_MIN_MEM		0x1000000
#endif

struct pci_dev;

/*
 * The PCI address space does equal the physical memory address space.  The
 * networking and block device layers use this boolean for bounce buffer
 * decisions.
 */
#define PCI_DMA_BUS_IS_PHYS	(1)

/*
 * Allocate and map kernel buffer using consistent mode DMA for a device.
 * hwdev should be valid struct pci_dev pointer for PCI devices,
 * NULL for PCI-like buses (ISA, EISA).
 * Returns non-NULL cpu-view pointer to the buffer if successful and
 * sets *dma_addrp to the pci side dma address as well, else *dma_addrp
 * is undefined.
 */
extern void *pci_alloc_consistent(struct pci_dev *hwdev, size_t size,
				  dma_addr_t *dma_handle);

/*
 * Free and unmap a consistent DMA buffer.
 * cpu_addr is what was returned from pci_alloc_consistent,
 * size must be the same as what as passed into pci_alloc_consistent,
 * and likewise dma_addr must be the same as what *dma_addrp was set to.
 *
 * References to the memory and mappings associated with cpu_addr/dma_addr
 * past this call are illegal.
 */
extern void pci_free_consistent(struct pci_dev *hwdev, size_t size,
				void *vaddr, dma_addr_t dma_handle);


#ifdef CONFIG_MAPPED_PCI_IO

extern dma_addr_t pci_map_single(struct pci_dev *hwdev, void *ptr, size_t size,
                                 int direction);
extern void pci_unmap_single(struct pci_dev *hwdev, dma_addr_t dma_addr,
                             size_t size, int direction);
extern int pci_map_sg(struct pci_dev *hwdev, struct scatterlist *sg, int nents,
                      int direction);
extern void pci_unmap_sg(struct pci_dev *hwdev, struct scatterlist *sg,
                         int nents, int direction);
extern void pci_dma_sync_single(struct pci_dev *hwdev, dma_addr_t dma_handle,
                                size_t size, int direction);
extern void pci_dma_sync_sg(struct pci_dev *hwdev, struct scatterlist *sg,
                            int nelems, int direction);

/* pci_unmap_{single,page} is not a nop, thus... */
#define DECLARE_PCI_UNMAP_ADDR(ADDR_NAME)	dma_addr_t ADDR_NAME;
#define DECLARE_PCI_UNMAP_LEN(LEN_NAME)		__u32 LEN_NAME;
#define pci_unmap_addr(PTR, ADDR_NAME)		((PTR)->ADDR_NAME)
#define pci_unmap_addr_set(PTR, ADDR_NAME, VAL)	(((PTR)->ADDR_NAME) = (VAL))
#define pci_unmap_len(PTR, LEN_NAME)		((PTR)->LEN_NAME)
#define pci_unmap_len_set(PTR, LEN_NAME, VAL)	(((PTR)->LEN_NAME) = (VAL))

#else /* CONFIG_MAPPED_PCI_IO  */

/*
 * Map a single buffer of the indicated size for DMA in streaming mode.
 * The 32-bit bus address to use is returned.
 *
 * Once the device is given the dma address, the device owns this memory
 * until either pci_unmap_single or pci_dma_sync_single is performed.
 */
static inline dma_addr_t pci_map_single(struct pci_dev *hwdev, void *ptr,
					size_t size, int direction)
{
	unsigned long addr = (unsigned long) ptr;

	if (direction == PCI_DMA_NONE)
		BUG();

	dma_cache_wback_inv(addr, size);

	return bus_to_baddr(hwdev->bus, __pa(ptr));
}

/*
 * Unmap a single streaming mode DMA translation.  The dma_addr and size
 * must match what was provided for in a previous pci_map_single call.  All
 * other usages are undefined.
 *
 * After this call, reads by the cpu to the buffer are guaranteed to see
 * whatever the device wrote there.
 */
static inline void pci_unmap_single(struct pci_dev *hwdev, dma_addr_t dma_addr,
				    size_t size, int direction)
{
	if (direction == PCI_DMA_NONE)
		BUG();

	if (direction != PCI_DMA_TODEVICE) {
		unsigned long addr;

		addr = baddr_to_bus(hwdev->bus, dma_addr) + PAGE_OFFSET;
		dma_cache_wback_inv(addr, size);
	}
}

/* pci_unmap_{page,single} is a nop so... */
#define DECLARE_PCI_UNMAP_ADDR(ADDR_NAME)
#define DECLARE_PCI_UNMAP_LEN(LEN_NAME)
#define pci_unmap_addr(PTR, ADDR_NAME)		(0)
#define pci_unmap_addr_set(PTR, ADDR_NAME, VAL)	do { } while (0)
#define pci_unmap_len(PTR, LEN_NAME)		(0)
#define pci_unmap_len_set(PTR, LEN_NAME, VAL)	do { } while (0)

/*
 * pci_{map,unmap}_single_page maps a kernel page to a dma_addr_t. identical
 * to pci_map_single, but takes a struct page instead of a virtual address
 */
static inline dma_addr_t pci_map_page(struct pci_dev *hwdev, struct page *page,
				      unsigned long offset, size_t size,
                                      int direction)
{
	unsigned long addr;

	if (direction == PCI_DMA_NONE)
		BUG();

	addr = (unsigned long) page_address(page) + offset;
	dma_cache_wback_inv(addr, size);

	return bus_to_baddr(hwdev->bus, page_to_phys(page) + offset);
}

static inline void pci_unmap_page(struct pci_dev *hwdev, dma_addr_t dma_address,
				  size_t size, int direction)
{
	if (direction == PCI_DMA_NONE)
		BUG();

	if (direction != PCI_DMA_TODEVICE) {
		unsigned long addr;

		addr = baddr_to_bus(hwdev->bus, dma_address) + PAGE_OFFSET;
		dma_cache_wback_inv(addr, size);
	}
}

/*
 * Map a set of buffers described by scatterlist in streaming
 * mode for DMA.  This is the scather-gather version of the
 * above pci_map_single interface.  Here the scatter gather list
 * elements are each tagged with the appropriate dma address
 * and length.  They are obtained via sg_dma_{address,length}(SG).
 *
 * NOTE: An implementation may be able to use a smaller number of
 *       DMA address/length pairs than there are SG table elements.
 *       (for example via virtual mapping capabilities)
 *       The routine returns the number of addr/length pairs actually
 *       used, at most nents.
 *
 * Device ownership issues as mentioned above for pci_map_single are
 * the same here.
 */
static inline int pci_map_sg(struct pci_dev *hwdev, struct scatterlist *sg,
			     int nents, int direction)
{
	int i;

	if (direction == PCI_DMA_NONE)
		BUG();

	for (i = 0; i < nents; i++, sg++) {
		unsigned long addr;

		addr = (unsigned long) page_address(sg->page);
		if (addr)
			dma_cache_wback_inv(addr + sg->offset, sg->length);
		sg->dma_address = (dma_addr_t) bus_to_baddr(hwdev->bus,
			page_to_phys(sg->page) + sg->offset);
	}

	return nents;
}

/*
 * Unmap a set of streaming mode DMA translations.
 * Again, cpu read rules concerning calls here are the same as for
 * pci_unmap_single() above.
 */
static inline void pci_unmap_sg(struct pci_dev *hwdev, struct scatterlist *sg,
				int nents, int direction)
{
	int i;

	if (direction == PCI_DMA_NONE)
		BUG();

	if (direction == PCI_DMA_TODEVICE)
		return;

	for (i = 0; i < nents; i++, sg++) {
		unsigned long addr;

		if (!sg->page)
			BUG();

		addr = (unsigned long) page_address(sg->page);
		if (addr)
			dma_cache_wback_inv(addr + sg->offset, sg->length);
	}
}

/*
 * Make physical memory consistent for a single
 * streaming mode DMA translation after a transfer.
 *
 * If you perform a pci_map_single() but wish to interrogate the
 * buffer using the cpu, yet do not wish to teardown the PCI dma
 * mapping, you must call this function before doing so.  At the
 * next point you give the PCI dma address back to the card, the
 * device again owns the buffer.
 */
static inline void pci_dma_sync_single(struct pci_dev *hwdev,
				       dma_addr_t dma_handle,
				       size_t size, int direction)
{
	unsigned long addr;

	if (direction == PCI_DMA_NONE)
		BUG();

	addr = baddr_to_bus(hwdev->bus, dma_handle) + PAGE_OFFSET;
	dma_cache_wback_inv(addr, size);
}

/*
 * Make physical memory consistent for a set of streaming
 * mode DMA translations after a transfer.
 *
 * The same as pci_dma_sync_single but for a scatter-gather list,
 * same rules and usage.
 */
static inline void pci_dma_sync_sg(struct pci_dev *hwdev,
				   struct scatterlist *sg,
				   int nelems, int direction)
{
#ifdef CONFIG_NONCOHERENT_IO
	int i;
#endif

	if (direction == PCI_DMA_NONE)
		BUG();

	/* Make sure that gcc doesn't leave the empty loop body.  */
#ifdef CONFIG_NONCOHERENT_IO
	for (i = 0; i < nelems; i++, sg++)
		dma_cache_wback_inv((unsigned long)page_address(sg->page),
		                    sg->length);
#endif
}
#endif /* CONFIG_MAPPED_PCI_IO  */

/*
 * Return whether the given PCI device DMA address mask can
 * be supported properly.  For example, if your device can
 * only drive the low 24-bits during PCI bus mastering, then
 * you would pass 0x00ffffff as the mask to this function.
 */
static inline int pci_dma_supported(struct pci_dev *hwdev, u64 mask)
{
	/*
	 * we fall back to GFP_DMA when the mask isn't all 1s,
	 * so we can't guarantee allocations that must be
	 * within a tighter range than GFP_DMA..
	 */
#ifdef CONFIG_ISA
	if (mask < 0x00ffffff)
		return 0;
#endif

	return 1;
}

/* This is always fine. */
#define pci_dac_dma_supported(pci_dev, mask)	(1)

static inline dma64_addr_t pci_dac_page_to_dma(struct pci_dev *pdev,
	struct page *page, unsigned long offset, int direction)
{
	dma64_addr_t addr = page_to_phys(page) + offset;

	return (dma64_addr_t) bus_to_baddr(pdev->bus, addr);
}

static inline struct page *pci_dac_dma_to_page(struct pci_dev *pdev,
	dma64_addr_t dma_addr)
{
	unsigned long poff = baddr_to_bus(pdev->bus, dma_addr) >> PAGE_SHIFT;

	return mem_map + poff;
}

static inline unsigned long pci_dac_dma_to_offset(struct pci_dev *pdev,
	dma64_addr_t dma_addr)
{
	return dma_addr & ~PAGE_MASK;
}

static inline void pci_dac_dma_sync_single(struct pci_dev *pdev,
	dma64_addr_t dma_addr, size_t len, int direction)
{
	unsigned long addr;

	if (direction == PCI_DMA_NONE)
		BUG();

	addr = baddr_to_bus(pdev->bus, dma_addr) + PAGE_OFFSET;
	dma_cache_wback_inv(addr, len);
}

#endif /* __KERNEL__ */

/* generic pci stuff */
#include <asm-generic/pci.h>

#endif /* _ASM_PCI_H */
