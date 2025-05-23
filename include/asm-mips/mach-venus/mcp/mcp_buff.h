#ifndef __MCP_BUFF_H__
#define __MCP_BUFF_H__

#define MCP_BUFF_HEAD_RESERVE            128
#define MCP_BUFF_TAIL_RESERVE            128
#define MCP_BUFF_RESERVE_AREA            (MCP_BUFF_HEAD_RESERVE + MCP_BUFF_TAIL_RESERVE)

typedef struct 
{
    unsigned char*   head;
    unsigned char*   data;
    unsigned char*   tail;
    unsigned char*   end;
    unsigned long    len;
    
    unsigned long    flag;
#define MCPB_FLAG_DVR_MALLOC             0x1UL
}mcp_buff;


mcp_buff*      alloc_mcpb    (unsigned int len);
void           free_mcpb     (mcp_buff* mcpb);
void           mcpb_reserve  (mcp_buff* mcpb, unsigned int len);
unsigned char* mcpb_put      (mcp_buff* mcpb, unsigned int len);
void           mcpb_trim     (mcp_buff* mcpb, unsigned int len);
unsigned char* mcpb_push     (mcp_buff* mcpb, unsigned int len);
unsigned char* mcpb_pull     (mcp_buff* mcpb, unsigned int len);
void           mcpb_purge    (mcp_buff* mcpb);
int            mcpb_headroom (mcp_buff* mcpb);
int            mcpb_tailroom (mcp_buff* mcpb);

void           mcpb_dump_data(mcp_buff* mcpb);
void           mcpb_dump_tail(mcp_buff* mcpb);

#endif  // __MCP_BUFF_H__
