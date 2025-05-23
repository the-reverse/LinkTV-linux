/*
 * Register Address for Venus/Neptune/Mars
 */
#define MIS_PSELL				((volatile unsigned int *)0xb801b004)
#define MIS_UMSK_ISR			((volatile unsigned int *)0xb801b008)
#define MIS_ISR					((volatile unsigned int *)0xb801b00c)
#define MIS_UMSK_ISR_KPADAH		((volatile unsigned int *)0xb801b018)
#define MIS_UMSK_ISR_KPADAL		((volatile unsigned int *)0xb801b01c)
#define MIS_UMSK_ISR_KPADDAH	((volatile unsigned int *)0xb801b020)
#define MIS_UMSK_ISR_KPADDAL	((volatile unsigned int *)0xb801b024)
#define MIS_UMSK_ISR_SW			((volatile unsigned int *)0xb801b028)
#define MIS_VFD_CTL				((volatile unsigned int *)0xb801b700)
#define MIS_VFD_WRCTL			((volatile unsigned int *)0xb801b704)
#define MIS_VFDO				((volatile unsigned int *)0xb801b708)
#define MIS_VFD_ARDCTL			((volatile unsigned int *)0xb801b70c)
#define MIS_VFD_KPADLIE			((volatile unsigned int *)0xb801b710)
#define MIS_VFD_KPADHIE			((volatile unsigned int *)0xb801b714)
#define MIS_VFD_SWIE			((volatile unsigned int *)0xb801b718)
#define MIS_VFD_ARDKPADL		((volatile unsigned int *)0xb801b71c)
#define MIS_VFD_ARDKPADH		((volatile unsigned int *)0xb801b720)
#define MIS_VFD_ARDSW			((volatile unsigned int *)0xb801b724)

/*
 * Register Address for Jupiter
 */
#define ISO_UMSK_ISR			((volatile unsigned int *)0xb801bd04)
#define ISO_ISR					((volatile unsigned int *)0xb801bd00)
#define ISO_UMSK_ISR_KPADAH		((volatile unsigned int *)0xb801bd0c)
#define ISO_UMSK_ISR_KPADAL		((volatile unsigned int *)0xb801bd10)
#define ISO_UMSK_ISR_KPADDAH	((volatile unsigned int *)0xb801bd14)
#define ISO_UMSK_ISR_KPADDAL	((volatile unsigned int *)0xb801bd18)
#define ISO_UMSK_ISR_SW			((volatile unsigned int *)0xb801bd1C)
#define ISO_VFD_CTL				((volatile unsigned int *)0xb801bda0)
#define ISO_VFD_WRCTL			((volatile unsigned int *)0xb801bda4)
#define ISO_VFDO				((volatile unsigned int *)0xb801bda8)
#define ISO_VFD_ARDCTL			((volatile unsigned int *)0xb801bdac)
#define ISO_VFD_KPADLIE			((volatile unsigned int *)0xb801bdb0)
#define ISO_VFD_KPADHIE			((volatile unsigned int *)0xb801bdb4)
#define ISO_VFD_SWIE			((volatile unsigned int *)0xb801bdb8)
#define ISO_VFD_ARDKPADL		((volatile unsigned int *)0xb801bdbc)
#define ISO_VFD_ARDKPADH		((volatile unsigned int *)0xb801bdc0)
#define ISO_VFD_ARDSW			((volatile unsigned int *)0xb801bdc4)
#define ISO_VFD_CTL1			((volatile unsigned int *)0xb801bdc8)


#define VENUS_VFD_IRQ   3       /* HW1 +2 = 3 */

#define VENUS_VFD_MAJOR			234
#define VENUS_VFD_DEVICE_NUM	3
#define VENUS_VFD_MINOR_VFDO	0
#define VENUS_VFD_MINOR_WRCTL	1
#define VENUS_VFD_MINOR_KEYPAD	2
#define VENUS_VFD_VFDO_DEVICE	"venus_vfdo"
#define VENUS_VFD_WRCTL			"venus_vfd_wrctl"
#define VENUS_VFD_KEYPAD		"venus_vfd_keypad"

#define VENUS_VFD_SWITCH_BASE	128

#define VENUS_VFD_IOC_MAGIC			'r'
#define VENUS_VFD_IOC_DISABLE_AUTOREAD	_IO(VENUS_VFD_IOC_MAGIC, 1)
#define VENUS_VFD_IOC_ENABLE_AUTOREAD	_IO(VENUS_VFD_IOC_MAGIC, 2)
#define VENUS_VFD_IOC_MAXNR			8
