/*
 *  linux/fs/affs/namei.c
 *
 *  (c) 1996  Hans-Joachim Widmaier - Rewritten
 *
 *  (C) 1993  Ray Burr - Modified for Amiga FFS filesystem.
 *
 *  (C) 1991  Linus Torvalds - minix filesystem
 */

#define DEBUG 0
#include <linux/sched.h>
#include <linux/affs_fs.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/fcntl.h>
#include <linux/locks.h>
#include <linux/amigaffs.h>
#include <asm/uaccess.h>

#include <linux/errno.h>

/* Simple toupper() for DOS\1 */

static inline unsigned int
affs_toupper(unsigned int ch)
{
	return ch >= 'a' && ch <= 'z' ? ch -= ('a' - 'A') : ch;
}

/* International toupper() for DOS\3 */

static inline unsigned int
affs_intl_toupper(unsigned int ch)
{
	return (ch >= 'a' && ch <= 'z') || (ch >= 0xE0
		&& ch <= 0xFE && ch != 0xF7) ?
		ch - ('a' - 'A') : ch;
}

/*
 * NOTE! unlike strncmp, affs_match returns 1 for success, 0 for failure.
 */

static int
affs_match(const unsigned char *name, int len, const unsigned char *compare, int dlen, int intl)
{
	if (!compare)
		return 0;

	if (len > 30)
		len = 30;
	if (dlen > 30)
		dlen = 30;

	/* "" means "." ---> so paths like "/usr/lib//libc.a" work */
	if (!len && dlen == 1 && compare[0] == '.')
		return 1;
	if (dlen != len)
		return 0;
	if (intl) {
		while (dlen--) {
			if (affs_intl_toupper(*name) != affs_intl_toupper(*compare))
				return 0;
			name++;
			compare++;
		}
	} else {
		while (dlen--) {
			if (affs_toupper(*name) != affs_toupper(*compare))
				return 0;
			name++;
			compare++;
		}
	}
	return 1;
}

int
affs_hash_name(const unsigned char *name, int len, int intl, int hashsize)
{
	unsigned int i, x;

	if (len > 30)
		len = 30;

	x = len;
	for (i = 0; i < len; i++)
		if (intl)
			x = (x * 13 + affs_intl_toupper(name[i] & 0xFF)) & 0x7ff;
		else
			x = (x * 13 + affs_toupper(name[i] & 0xFF)) & 0x7ff;

	return x % hashsize;
}

static struct buffer_head *
affs_find_entry(struct inode *dir, struct dentry *dentry, unsigned long *ino)
{
	struct buffer_head	*bh;
	int			 intl;
	s32			 key;
	const char		*name = dentry->d_name.name;
	int			 namelen = dentry->d_name.len;

	pr_debug("AFFS: find_entry(\"%.*s\")\n",namelen,name);

	intl = AFFS_I2FSTYPE(dir);
	bh   = affs_bread(dir->i_dev,dir->i_ino,AFFS_I2BSIZE(dir));
	if (!bh)
		return NULL;

	if (affs_match(name,namelen,".",1,intl)) {
		*ino = dir->i_ino;
		return bh;
	}
	if (affs_match(name,namelen,"..",2,intl)) {
		*ino = affs_parent_ino(dir);
		return bh;
	}

	key = AFFS_GET_HASHENTRY(bh->b_data,affs_hash_name(name,namelen,intl,AFFS_I2HSIZE(dir)));

	for (;;) {
		unsigned char *cname;
		int cnamelen;

		affs_brelse(bh);
		if (key == 0) {
			bh = NULL;
			break;
		}
		bh = affs_bread(dir->i_dev,key,AFFS_I2BSIZE(dir));
		if (!bh)
			break;
		cnamelen = affs_get_file_name(AFFS_I2BSIZE(dir),bh->b_data,&cname);
		if (affs_match(name,namelen,cname,cnamelen,intl))
			break;
		key = be32_to_cpu(FILE_END(bh->b_data,dir)->hash_chain);
	}
	*ino = key;
	return bh;
}

int
affs_lookup(struct inode *dir, struct dentry *dentry)
{
	unsigned long		 ino;
	struct buffer_head	*bh;
	struct inode		*inode;

	pr_debug("AFFS: lookup(\"%.*s\")\n",(int)dentry->d_name.len,dentry->d_name.name);

	inode = NULL;
	bh = affs_find_entry(dir,dentry,&ino);
	if (bh) {
		if (FILE_END(bh->b_data,dir)->original)
			ino = be32_to_cpu(FILE_END(bh->b_data,dir)->original);
		affs_brelse(bh);
		inode = iget(dir->i_sb,ino);
		if (!inode)
			return -EACCES;
	}
	d_add(dentry,inode);
	return 0;
}

int
affs_unlink(struct inode *dir, struct dentry *dentry)
{
	int			 retval;
	struct buffer_head	*bh;
	unsigned long		 ino;
	struct inode		*inode;

	pr_debug("AFFS: unlink(dir=%ld,\"%.*s\")\n",dir->i_ino,
		 (int)dentry->d_name.len,dentry->d_name.name);

	bh      = NULL;
	retval  = -ENOENT;
	if (!dir)
		goto unlink_done;
	if (!(bh = affs_find_entry(dir,dentry,&ino)))
		goto unlink_done;

	inode  = dentry->d_inode;
	retval = -EPERM;
	if (S_ISDIR(inode->i_mode))
		goto unlink_done;
	if (current->fsuid != inode->i_uid &&
	    current->fsuid != dir->i_uid && !fsuser())
		goto unlink_done;

	if ((retval = affs_remove_header(bh,inode)) < 0)
		goto unlink_done;
	
	inode->i_nlink = retval;
	inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	mark_inode_dirty(inode);
	retval = 0;
	d_delete(dentry);
unlink_done:
	affs_brelse(bh);
	return retval;
}

int
affs_create(struct inode *dir, struct dentry *dentry, int mode)
{
	struct inode	*inode;
	int		 error;
	
	pr_debug("AFFS: create(%lu,\"%.*s\",0%o)\n",dir->i_ino,(int)dentry->d_name.len,
		 dentry->d_name.name,mode);

	if (!dir)
		return -ENOENT;
	inode = affs_new_inode(dir);
	if (!inode)
		return -ENOSPC;

	pr_debug(" -- ino=%lu\n",inode->i_ino);
	if (dir->i_sb->u.affs_sb.s_flags & SF_OFS)
		inode->i_op = &affs_file_inode_operations_ofs;
	else
		inode->i_op = &affs_file_inode_operations;

	error = affs_add_entry(dir,NULL,inode,dentry,ST_FILE);
	if (error) {
		inode->i_nlink = 0;
		mark_inode_dirty(inode);
		iput(inode);
		return error;
	}
	inode->i_mode = mode;
	inode->u.affs_i.i_protect = mode_to_prot(inode->i_mode);
	dir->i_version = ++event;
	mark_inode_dirty(dir);
	d_instantiate(dentry,inode);

	return 0;
}

int
affs_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
	struct inode		*inode;
	int			 error;
	
	pr_debug("AFFS: mkdir(%lu,\"%.*s\",0%o)\n",dir->i_ino,
		 (int)dentry->d_name.len,dentry->d_name.name,mode);

	inode = affs_new_inode(dir);
	if (!inode)
		return -ENOSPC;

	inode->i_op = &affs_dir_inode_operations;
	error       = affs_add_entry(dir,NULL,inode,dentry,ST_USERDIR);
	if (error) {
		inode->i_nlink = 0;
		mark_inode_dirty(inode);
		iput(inode);
		return error;
	}
	inode->i_mode = S_IFDIR | (mode & 0777 & ~current->fs->umask);
	inode->u.affs_i.i_protect = mode_to_prot(inode->i_mode);
	dir->i_version = ++event;
	mark_inode_dirty(dir);
	d_instantiate(dentry,inode);

	return 0;
}

static int
empty_dir(struct buffer_head *bh, int hashsize)
{
	while (--hashsize >= 0) {
		if (((struct dir_front *)bh->b_data)->hashtable[hashsize])
			return 0;
	}
	return 1;
}

int
affs_rmdir(struct inode *dir, struct dentry *dentry)
{
	int			 retval;
	unsigned long		 ino;
	struct inode		*inode;
	struct buffer_head	*bh;

	pr_debug("AFFS: rmdir(dir=%lu,\"%.*s\")\n",dir->i_ino,
		 (int)dentry->d_name.len,dentry->d_name.name);

	inode  = NULL;
	bh     = NULL;
	retval = -ENOENT;
	if (!dir)
		goto rmdir_done;
	if (!(bh = affs_find_entry(dir,dentry,&ino)))
		goto rmdir_done;

	inode = dentry->d_inode;

	retval = -EPERM;
        if (current->fsuid != inode->i_uid &&
            current->fsuid != dir->i_uid && !fsuser())
		goto rmdir_done;
	if (inode->i_dev != dir->i_dev)
		goto rmdir_done;
	if (inode == dir)	/* we may not delete ".", but "../dir" is ok */
		goto rmdir_done;
	if (!S_ISDIR(inode->i_mode)) {
		retval = -ENOTDIR;
		goto rmdir_done;
	}
	down(&inode->i_sem);
	if (dentry->d_count > 1) {
		shrink_dcache_parent(dentry);
	}
	up(&inode->i_sem);
	if (!empty_dir(bh,AFFS_I2HSIZE(inode))) {
		retval = -ENOTEMPTY;
		goto rmdir_done;
	}
	if (inode->i_count > 1) {
		retval = -EBUSY;
		goto rmdir_done;
	}
	if ((retval = affs_remove_header(bh,inode)) < 0)
		goto rmdir_done;
	
	inode->i_nlink = retval;
	inode->i_ctime = dir->i_ctime = dir->i_mtime = CURRENT_TIME;
	retval         = 0;
	mark_inode_dirty(inode);
	d_delete(dentry);
rmdir_done:
	affs_brelse(bh);
	return retval;
}

int
affs_symlink(struct inode *dir, struct dentry *dentry, const char *symname)
{
	struct buffer_head	*bh;
	struct inode		*inode;
	char			*p;
	unsigned long		 tmp;
	int			 i, maxlen;
	char			 c, lc;

	pr_debug("AFFS: symlink(%lu,\"%.*s\" -> \"%s\")\n",dir->i_ino,
		 (int)dentry->d_name.len,dentry->d_name.name,symname);
	
	maxlen = 4 * AFFS_I2HSIZE(dir) - 1;
	inode  = affs_new_inode(dir);
	if (!inode)
		return -ENOSPC;

	inode->i_op   = &affs_symlink_inode_operations;
	inode->i_mode = S_IFLNK | 0777;
	inode->u.affs_i.i_protect = mode_to_prot(inode->i_mode);
	bh = affs_bread(inode->i_dev,inode->i_ino,AFFS_I2BSIZE(inode));
	if (!bh) {
		inode->i_nlink = 0;
		mark_inode_dirty(inode);
		iput(inode);
		return -EIO;
	}
	i  = 0;
	p  = ((struct slink_front *)bh->b_data)->symname;
	lc = '/';
	if (*symname == '/') {
		while (*symname == '/')
			symname++;
		while (inode->i_sb->u.affs_sb.s_volume[i])	/* Cannot overflow */
			*p++ = inode->i_sb->u.affs_sb.s_volume[i++];
	}
	while (i < maxlen && (c = *symname++)) {
		if (c == '.' && lc == '/' && *symname == '.' && symname[1] == '/') {
			*p++ = '/';
			i++;
			symname += 2;
			lc = '/';
		} else if (c == '.' && lc == '/' && *symname == '/') {
			symname++;
			lc = '/';
		} else {
			*p++ = c;
			lc   = c;
			i++;
		}
		if (lc == '/')
			while (*symname == '/')
				symname++;
	}
	*p = 0;
	mark_buffer_dirty(bh,1);
	affs_brelse(bh);
	mark_inode_dirty(inode);
	bh = affs_find_entry(dir,dentry,&tmp);
	if (bh) {
		inode->i_nlink = 0;
		iput(inode);
		affs_brelse(bh);
		return -EEXIST;
	}
	i = affs_add_entry(dir,NULL,inode,dentry,ST_SOFTLINK);
	if (i) {
		inode->i_nlink = 0;
		mark_inode_dirty(inode);
		iput(inode);
		affs_brelse(bh);
		return i;
	}
	dir->i_version = ++event;
	d_instantiate(dentry,inode);
	
	return 0;
}

int
affs_link(struct inode *oldinode, struct inode *dir, struct dentry *dentry)
{
	struct inode		*inode;
	struct buffer_head	*bh;
	unsigned long		 i;
	int			 error;
	
	pr_debug("AFFS: link(%lu,%lu,\"%.*s\")\n",oldinode->i_ino,dir->i_ino,
		 (int)dentry->d_name.len,dentry->d_name.name);

	bh = affs_find_entry(dir,dentry,&i);
	if (bh) {
		affs_brelse(bh);
		return -EEXIST;
	}
	if (oldinode->u.affs_i.i_hlink)	{	/* Cannot happen */
		affs_warning(dir->i_sb,"link","Impossible link to link");
		return -EINVAL;
	}
	if (!(inode = affs_new_inode(dir)))
		return -ENOSPC;

	inode->i_op                = oldinode->i_op;
	inode->u.affs_i.i_protect  = mode_to_prot(oldinode->i_mode);
	inode->u.affs_i.i_original = oldinode->i_ino;
	inode->u.affs_i.i_hlink    = 1;
	inode->i_mtime             = oldinode->i_mtime;

	if (S_ISDIR(oldinode->i_mode))
		error = affs_add_entry(dir,oldinode,inode,dentry,ST_LINKDIR);
	else
		error = affs_add_entry(dir,oldinode,inode,dentry,ST_LINKFILE);
	if (error)
		inode->i_nlink = 0;
	else {
		dir->i_version = ++event;
		mark_inode_dirty(oldinode);
		oldinode->i_count++;
		d_instantiate(dentry,oldinode);
	}
	mark_inode_dirty(inode);
	iput(inode);

	return error;
}

/* This is copied from the ext2 fs. No need to reinvent the wheel. */

static int
subdir(struct dentry * new_dentry, struct dentry * old_dentry)
{
	int result;

	result = 0;
	for (;;) {
		if (new_dentry != old_dentry) {
			struct dentry * parent = new_dentry->d_parent;
			if (parent == new_dentry)
				break;
			new_dentry = parent;
			continue;
		}
		result = 1;
		break;
	}
	return result;
}

int
affs_rename(struct inode *old_dir, struct dentry *old_dentry,
	    struct inode *new_dir, struct dentry *new_dentry)
{
	struct inode		*old_inode = old_dentry->d_inode;
	struct inode		*new_inode = new_dentry->d_inode;
	struct buffer_head	*old_bh;
	struct buffer_head	*new_bh;
	unsigned long		 old_ino;
	unsigned long		 new_ino;
	int			 retval;

	pr_debug("AFFS: rename(old=%lu,\"%*s\" (inode=%p) to new=%lu,\"%*s\" (inode=%p) )\n",
		 old_dir->i_ino,old_dentry->d_name.len,old_dentry->d_name.name,old_inode,
		 new_dir->i_ino,new_dentry->d_name.len,new_dentry->d_name.name,new_inode);
	
	if ((retval = affs_check_name(new_dentry->d_name.name,new_dentry->d_name.len)))
		return retval;

	new_bh = NULL;
	retval = -ENOENT;
	old_bh = affs_find_entry(old_dir,old_dentry,&old_ino);
	if (!old_bh)
		goto end_rename;

	new_bh = affs_find_entry(new_dir,new_dentry,&new_ino);
	if (new_bh && !new_inode) {
		affs_error(old_inode->i_sb,"affs_rename",
			   "No inode for entry found (key=%lu)\n",new_ino);
		goto end_rename;
	}
	if (new_inode == old_inode) {
		if (old_ino == new_ino) {	/* Filename might have changed case	*/
			retval = new_dentry->d_name.len < 31 ? new_dentry->d_name.len : 30;
			strncpy(DIR_END(old_bh->b_data,old_inode)->dir_name + 1,
				new_dentry->d_name.name,retval);
			DIR_END(old_bh->b_data,old_inode)->dir_name[0] = retval;
			goto new_checksum;
		}
		retval = 0;
		goto end_rename;
	}
	if (new_inode && S_ISDIR(new_inode->i_mode)) {
		retval = -EISDIR;
		if (!S_ISDIR(old_inode->i_mode))
			goto end_rename;
		retval = -EINVAL;
		if (subdir(new_dentry,old_dentry))
			goto end_rename;
		retval = -ENOTEMPTY;
		if (!empty_dir(new_bh,AFFS_I2HSIZE(new_inode)))
			goto end_rename;
		retval = -EBUSY;
		if (new_dentry->d_count > 1)
			goto end_rename;
	}
	if (S_ISDIR(old_inode->i_mode)) {
		retval = -ENOTDIR;
		if (new_inode && !S_ISDIR(new_inode->i_mode))
			goto end_rename;
		retval = -EINVAL;
		if (subdir(new_dentry,old_dentry))
			goto end_rename;
		if (affs_parent_ino(old_inode) != old_dir->i_ino)
			goto end_rename;
	}
	/* Unlink destination if it already exists */
	if (new_inode) {
		if ((retval = affs_remove_header(new_bh,new_dir)) < 0)
			goto end_rename;
		new_inode->i_nlink = retval;
		mark_inode_dirty(new_inode);
		if (new_inode->i_ino == new_ino)
			new_inode->i_nlink = 0;
	}
	/* Remove header from its parent directory. */
	if ((retval = affs_remove_hash(old_bh,old_dir)))
		goto end_rename;
	/* And insert it into the new directory with the new name. */
	affs_copy_name(FILE_END(old_bh->b_data,old_inode)->file_name,new_dentry->d_name.name);
	if ((retval = affs_insert_hash(new_dir->i_ino,old_bh,new_dir)))
		goto end_rename;
new_checksum:
	affs_fix_checksum(AFFS_I2BSIZE(new_dir),old_bh->b_data,5);

	new_dir->i_ctime   = new_dir->i_mtime = old_dir->i_ctime
			   = old_dir->i_mtime = CURRENT_TIME;
	new_dir->i_version = ++event;
	old_dir->i_version = ++event;
	retval             = 0;
	mark_inode_dirty(new_dir);
	mark_inode_dirty(old_dir);
	mark_buffer_dirty(old_bh,1);
	d_move(old_dentry,new_dentry);
	
end_rename:
	affs_brelse(old_bh);
	affs_brelse(new_bh);

	return retval;
}
