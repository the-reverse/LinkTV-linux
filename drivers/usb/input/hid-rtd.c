/*
 *  HID driver for Realtek HID input devices
 *
 *  Copyright (c) 2011 Ching-Yu Huang
 */

#include <linux/types.h>
#include <linux/input.h>

#include "hid.h"
#include <linux/rtd_hid_keymap.h>

/**
 * hid_map_usage - map usage input bits
 *
 * @hidinput: hidinput which we are interested in
 * @usage: usage to fill in
 * @bit: pointer to input->{}bit (out parameter)
 * @max: maximal valid usage->code to consider later (out parameter)
 * @type: input event type (EV_KEY, EV_REL, ...)
 * @c: code which corresponds to this usage and type
 */
static inline void hid_map_usage(struct hid_input *hidinput,
        struct hid_usage *usage, unsigned long **bit, int *max,
        __u8 type, __u16 c)
{
    struct input_dev *input = &hidinput->input;

    usage->type = type;
    usage->code = c;

    switch (type) {
    case EV_ABS:
        *bit = input->absbit;
        *max = ABS_MAX;
        break;
    case EV_REL:
        *bit = input->relbit;
        *max = REL_MAX;
        break;
    case EV_KEY:
        *bit = input->keybit;
        *max = KEY_MAX;
        break;
    case EV_LED:
        *bit = input->ledbit;
        *max = LED_MAX;
        break;
    }
}
                                           
/**
 * hid_map_usage_clear - map usage input bits and clear the input bit
 *
 * The same as hid_map_usage, except the @c bit is also cleared in supported
 * bits (@bit).
 */
static inline void hid_map_usage_clear(struct hid_input *hidinput,
        struct hid_usage *usage, unsigned long **bit, int *max,
        __u8 type, __u16 c)
{
    hid_map_usage(hidinput, usage, bit, max, type, c);
    clear_bit(c, *bit);
}
  
#define rtd_map_key_clear(c)	hid_map_usage_clear(hi, usage, bit, max, \
					EV_KEY, (c)) 

static int rtd_mce_input_mapping(struct hid_input *hi,struct hid_field *field, struct hid_usage *usage,unsigned long **bit, int *max)
{
    if ((usage->hid & HID_USAGE_PAGE) != 0x0ffbc0000)
        return 0;
    
    // printk("%s : usage->hid & HID_USAGE = 0x%x\n",__func__,(usage->hid & HID_USAGE));
        
    switch (usage->hid & HID_USAGE) 
    {
        // Launch App
        case 0x00d: 
            rtd_map_key_clear(RTD_MCE_KEY_HOME);            
        break;
        // DVD Menu
        case 0x024: 
            rtd_map_key_clear(RTD_MCE_KEY_DVD_MENU);           
        break;
        // Live TV
        case 0x025: 
            rtd_map_key_clear(RTD_MCE_KEY_TV);             
        break;
        // Zoom 
        case 0x027: 
            rtd_map_key_clear(RTD_MCE_KEY_ZOOM);             
        break;
        // Eject
        case 0x028: 
            rtd_map_key_clear(RTD_MCE_KEY_EJECTCD);             
        break;
        // Closed Captioning 
        case 0x02b: 
            rtd_map_key_clear(RTD_MCE_KEY_CLOSED_CAPTIONING);             
        break;              
        case 0x031: 
            rtd_map_key_clear(RTD_MCE_KEY_AUDIO);          
        break;        
        // TEXT
        case 0x032: 
            rtd_map_key_clear(RTD_MCE_KEY_TEXT);           
        break;
        // Channel
        case 0x033: 
            rtd_map_key_clear(RTD_MCE_KEY_CHANNEL);        
        break;
        // DVD Top Menu         
        case 0x043: 
            rtd_map_key_clear(RTD_MCE_KEY_DVD_TOP_MENU);            
        break;   
        /* 
        // Green
        case 0x047: 
            rtd_map_key_clear(RTD_MCE_KEY_GREEN);          
        break;
        */
        // Recorded TV
        case 0x048: 
            rtd_map_key_clear(RTD_MCE_KEY_REC_TV);            
        break;    
        /*            
        // Yellow
        case 0x049: 
            rtd_map_key_clear(RTD_MCE_KEY_YELLOW);         
        break;
        // Blue
        case 0x04a: 
            rtd_map_key_clear(RTD_MCE_KEY_BLUE);           
        break;       
        */
        // DVD Angle
        case 0x04b: 
            rtd_map_key_clear(RTD_MCE_KEY_DVD_ANGLE);          
        break;
        // DVD Audio
        case 0x04c: 
            rtd_map_key_clear(RTD_MCE_KEY_DVD_AUDIO);       
        break;
        // DVD Subtitle 
        case 0x04d: 
            rtd_map_key_clear(RTD_MCE_KEY_DVD_SUBTITLE);       
        break;       
        // TeleText On/Off
        case 0x05a: 
            rtd_map_key_clear(RTD_MCE_KEY_TELETEXT_ONOFF);          
        break;            
        // TeleText Red 
        case 0x05b: 
            rtd_map_key_clear(RTD_MCE_KEY_TELETEXT_RED);          
        break;                
        // TeleText Green
        case 0x05c: 
            rtd_map_key_clear(RTD_MCE_KEY_TELETEXT_GREEN);          
        break;              
        // TeleText Yellow 
        case 0x05d: 
            rtd_map_key_clear(RTD_MCE_KEY_TELETEXT_YELLOW);          
        break;              
        // TeleText Blue  
        case 0x05e: 
            rtd_map_key_clear(RTD_MCE_KEY_TELETEXT_BLUE);          
        break;        
        default:
            return 0;
        break;
    }  
    
    return 1;      
}

static int rtd_consumer_input_mapping(struct hid_input *hi,struct hid_field *field, struct hid_usage *usage,unsigned long **bit, int *max)
{
    if ((usage->hid & HID_USAGE_PAGE) != HID_UP_CONSUMER)
        return 0;
    
    // printk("%s : usage->hid & HID_USAGE = 0x%x\n",__func__,(usage->hid & HID_USAGE));
    
    switch (usage->hid & HID_USAGE) 
    {
        // Media Select Program Guide   
        case 0x8D : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_GUIDE);            
        break;
        // Help    
        case 0x95 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_HELP);            
        break;
        // Chan/Page Up   
        case 0x9C : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_CHAN_PAGE_UP);            
        break;           
        // Chan/Page Down    
        case 0x9D : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_CHAN_PAGE_DOWN);            
        break;  
        // Play
        case 0xB0: 
            rtd_map_key_clear(RTD_CONSUMER_KEY_PLAY);            
        break;
        // Pause 
        case 0xB1: 
            rtd_map_key_clear(RTD_CONSUMER_KEY_PAUSE);            
        break;
        // Record 
        case 0xB2: 
            rtd_map_key_clear(RTD_CONSUMER_KEY_RECORD);            
        break;
        // Fast Forward
        case 0xB3 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_FAST_FORWARD);            
        break;
        // Rewind
        case 0xB4 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_REWIND);            
        break;
        // Skip Forward 
        case 0xB5 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_SKIP_FORWARD);            
        break;
        // Skip Back  
        case 0xB6 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_SKIP_BACK);            
        break;
        // Stop
        case 0xB7: 
            rtd_map_key_clear(RTD_CONSUMER_KEY_STOP);            
        break; 
        // Eject
        case 0xB8: 
            rtd_map_key_clear(RTD_CONSUMER_KEY_EJECT);            
        break; 
        // Repeat
        case 0xBC: 
            rtd_map_key_clear(RTD_CONSUMER_KEY_REPEAT);            
        break;
        // Play/Pause Combo 
        case 0xCD: 
            rtd_map_key_clear(RTD_CONSUMER_KEY_PLAY_PAUSE_COMBO);            
        break;  
        // Play/Skip Combo 
        case 0xCE: 
            rtd_map_key_clear(RTD_CONSUMER_KEY_PLAY_SKIP_COMBO);            
        break;  
        // Volume
        case 0xE0 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_VOLUME);            
        break; 
        // Mute  
        case 0xE2 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_MUTE);            
        break;
        // Bass Boost  
        case 0xE5 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_BASS_BOOST);            
        break;
        // Loudness
        case 0xE7 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_LOUDNESS);            
        break;                                            
        // Volume Up  
        case 0xE9 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_VOLUME_UP);            
        break;
        // Volume Down   
        case 0xEA : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_VOLUME_DOWN);            
        break;                                                                            
        // Bass Up
        case 0x0152 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_BASS_UP);            
        break;
        // Bass Down
        case 0x0153 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_BASS_DOWN);            
        break;
        // Treble Up
        case 0x0154 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_TREBLE_UP);            
        break;
        // Treble Down
        case 0x0155 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_TREBLE_DOWN);            
        break;
        // Media Select
        case 0x0183 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_MEDIA_SELECT);            
        break;
        // Mail
        case 0x018A : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_MAIL);            
        break;
        // Calculator
        case 0x0192 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_CALCULATOR);            
        break;
        // My Computer
        case 0x0194 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_MY_COMPUTER);            
        break;
        // More Info 
        case 0x0209: 
            rtd_map_key_clear(RTD_CONSUMER_KEY_MORE_INFO);            
        break;
        // WWW Search
        case 0x0221 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_WWW_SEARCH);            
        break;        
        // WWW Home
        case 0x0223 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_WWW_HOME);            
        break;
        // WWW Back
        case 0x0224 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_WWW_BACK);            
        break;
        // WWW Forward
        case 0x0225 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_WWW_FORWARD);            
        break;
        // WWW Stop
        case 0x0226 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_WWW_STOP);            
        break;
        // WWW Refresh
        case 0x0227 : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_WWW_REFRESH);            
        break;
        // WWW Favorites
        case 0x022A : 
            rtd_map_key_clear(RTD_CONSUMER_KEY_WWW_FAVORITES);            
        break;        
        default:
            return 0;
        break;   
    }    
        
    return 1;        
}        


static int rtd_gendesk_input_mapping(struct hid_input *hi,struct hid_field *field, struct hid_usage *usage,unsigned long **bit, int *max)
{
    if ((usage->hid & HID_USAGE_PAGE) != HID_UP_GENDESK)
        return 0;
    
    // printk("%s : usage->hid & HID_USAGE = 0x%x\n",__func__,(usage->hid & HID_USAGE));
    
    switch (usage->hid & HID_USAGE) 
    {
        // System Power
        case 0x81 : 
            rtd_map_key_clear(RTD_GENDESK_KEY_SYSTEM_POWER);            
        break;  
        // Power toggle (standby/sleep) 
        case 0x82 : 
            rtd_map_key_clear(RTD_GENDESK_KEY_SYSTEM_SLEEP);            
        break;
        // System Wake
        case 0x83 : 
            rtd_map_key_clear(RTD_GENDESK_KEY_SYSTEM_WAKE);            
        break;
        default:
            return 0;
        break;        
    }
    
    return 1;
}

int Rtd_Input_Mapping(struct hid_device *hdev, struct hid_input *hi,struct hid_field *field, struct hid_usage *usage,unsigned long **bit, int *max)
{
       
    if (rtd_mce_input_mapping(hi,field,usage,bit,max) == 1)
        return 1;
    else if (rtd_consumer_input_mapping(hi,field,usage,bit,max) == 1) 
        return 1;
    else if (rtd_gendesk_input_mapping(hi,field,usage,bit,max) == 1)    
        return 1;
    else
        return 0;        
}
