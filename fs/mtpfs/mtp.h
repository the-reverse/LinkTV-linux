#ifndef _FS_MTP_H_
#define _FS_MTP_H_

extern uint16_t mtpfs_ptp_getobjecthandles (struct mtpfs_sb_info *sb, uint32_t storage,
                                   uint32_t objectformatcode, uint32_t associationOH,
                                   PTPObjectHandles *objecthandles);

extern void mtpfs_ptp_free_object_handles(PTPObjectHandles *objecthandles);

extern uint16_t mtpfs_ptp_getobjectinfo (struct mtpfs_sb_info *sb, uint32_t handle,PTPObjectInfo *objectinfo);
extern void mtpfs_ptp_free_object_info(PTPObjectInfo *object);

extern uint16_t mtpfs_ptp_sendobjectinfo (struct mtpfs_sb_info *sb, uint32_t *store,
                                 uint32_t *parenthandle, uint32_t *handle,
                                 PTPObjectInfo *objectinfo);
extern uint16_t mtpfs_ptp_sendobject(struct mtpfs_sb_info *sb, unsigned char *buf, uint32_t size);
extern uint16_t mtpfs_ptp_deleteobject(struct mtpfs_sb_info *sb,uint32_t handle, uint32_t ofc);    
extern uint16_t mtpfs_ptp_getobject_session (struct mtpfs_sb_info *sb, uint32_t handle, unsigned char *buf, uint32_t offset,uint32_t size);
 
// extern bool mtpfs_is_mtp_device(struct mtpfs_sb_info *sb);                            
extern bool mtpfs_is_high_speed_device(struct mtpfs_sb_info *sb);
extern bool mtpfs_ptp_operation_issupported(struct mtpfs_sb_info *sb,uint16_t operation);
#endif // _FS_MTP_H_
