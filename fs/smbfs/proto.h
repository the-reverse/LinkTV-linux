/*
 *  Autogenerated with cproto on:  Tue Oct 2 20:40:54 CEST 2001
 */

/* proc.c */
extern int smb_setcodepage(struct smb_sb_info *server, struct smb_nls_codepage *cp);
extern __u32 smb_len(__u8 *p);
extern int smb_get_rsize(struct smb_sb_info *server);
extern int smb_get_wsize(struct smb_sb_info *server);
extern int smb_errno(struct smb_sb_info *server);
extern int smb_newconn(struct smb_sb_info *server, struct smb_conn_opt *opt);
extern int smb_wakeup(struct smb_sb_info *server);
extern __u8 *smb_setup_header(struct smb_sb_info *server, __u8 command, __u16 wct, __u16 bcc);
extern int smb_open(struct dentry *dentry, int wish);
extern int smb_close(struct inode *ino);
extern int smb_close_fileid(struct dentry *dentry, __u16 fileid);
extern int smb_proc_read(struct inode *inode, off_t offset, int count, char *data);
extern int smb_proc_write(struct inode *inode, off_t offset, int count, const char *data);
extern int smb_proc_create(struct dentry *dentry, __u16 attr, time_t ctime, __u16 *fileid);
extern int smb_proc_mv(struct dentry *old_dentry, struct dentry *new_dentry);
extern int smb_proc_mkdir(struct dentry *dentry);
extern int smb_proc_rmdir(struct dentry *dentry);
extern int smb_proc_unlink(struct dentry *dentry);
extern int smb_proc_flush(struct smb_sb_info *server, __u16 fileid);
extern int smb_proc_trunc(struct smb_sb_info *server, __u16 fid, __u32 length);
extern void smb_init_root_dirent(struct smb_sb_info *server, struct smb_fattr *fattr);
extern int smb_proc_readdir(struct file *filp, void *dirent, filldir_t filldir, struct smb_cache_control *ctl);
extern int smb_proc_getattr(struct dentry *dir, struct smb_fattr *fattr);
extern int smb_proc_setattr(struct dentry *dir, struct smb_fattr *fattr);
extern int smb_proc_settime(struct dentry *dentry, struct smb_fattr *fattr);
extern int smb_proc_dskattr(struct super_block *sb, struct statfs *attr);
/* dir.c */
extern struct file_operations smb_dir_operations;
extern struct inode_operations smb_dir_inode_operations;
extern void smb_new_dentry(struct dentry *dentry);
extern void smb_renew_times(struct dentry *dentry);
/* cache.c */
extern void smb_invalid_dir_cache(struct inode *dir);
extern void smb_invalidate_dircache_entries(struct dentry *parent);
extern struct dentry *smb_dget_fpos(struct dentry *dentry, struct dentry *parent, unsigned long fpos);
extern int smb_fill_cache(struct file *filp, void *dirent, filldir_t filldir, struct smb_cache_control *ctrl, struct qstr *qname, struct smb_fattr *entry);
/* sock.c */
extern int smb_valid_socket(struct inode *inode);
extern int smb_catch_keepalive(struct smb_sb_info *server);
extern int smb_dont_catch_keepalive(struct smb_sb_info *server);
extern void smb_close_socket(struct smb_sb_info *server);
extern int smb_round_length(int len);
extern int smb_request(struct smb_sb_info *server);
extern int smb_trans2_request(struct smb_sb_info *server, __u16 trans2_command, int ldata, unsigned char *data, int lparam, unsigned char *param, int *lrdata, unsigned char **rdata, int *lrparam, unsigned char **rparam);
/* inode.c */
extern struct inode *smb_iget(struct super_block *sb, struct smb_fattr *fattr);
extern void smb_get_inode_attr(struct inode *inode, struct smb_fattr *fattr);
extern void smb_set_inode_attr(struct inode *inode, struct smb_fattr *fattr);
extern void smb_invalidate_inodes(struct smb_sb_info *server);
extern int smb_revalidate_inode(struct dentry *dentry);
extern int smb_notify_change(struct dentry *dentry, struct iattr *attr);
/* file.c */
extern struct address_space_operations smb_file_aops;
extern struct file_operations smb_file_operations;
extern struct inode_operations smb_file_inode_operations;
/* ioctl.c */
extern int smb_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
