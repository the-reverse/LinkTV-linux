#ifndef RTK_DMP_PATCH
#define RTK_DMP_PATCH

#include <linux/timer.h>


static inline void setup_timer(struct timer_list * timer,
                                void (*function)(unsigned long),
                                unsigned long data)
{
        timer->function = function;
        timer->data = data;
        init_timer(timer);
}

#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
    
#endif //RTK_DMP_PATCH
