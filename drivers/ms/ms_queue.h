#ifndef MS_QUEUE_H
#define MS_QUEUE_H

struct request;
struct task_struct;

struct ms_queue {
	struct ms_card		*card;
	struct completion	thread_complete;
	wait_queue_head_t	thread_wq;
	struct semaphore	thread_sem;
	unsigned int		flags;
	struct request		*req;
	int			(*prep_fn)(struct ms_queue *, struct request *);
	int			(*issue_fn)(struct ms_queue *, struct request *);
	void			*data;
	struct request_queue	*queue;
	struct scatterlist	*sg;
};

struct ms_io_request {
	struct request		*rq;
	int			num;
	//struct ms_command	selcmd;		/* ms_queue private */
	//struct ms_command	cmd[4];		/* max 4 commands */
};

extern int ms_init_queue(struct ms_queue *, struct ms_card *, spinlock_t *);
extern void ms_cleanup_queue(struct ms_queue *);
extern void ms_queue_suspend(struct ms_queue *);
extern void ms_queue_resume(struct ms_queue *);

#endif
