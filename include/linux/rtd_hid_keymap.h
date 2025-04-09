#ifndef __RTD_HID_KEYMAP_H__
#define __RTD_HID_KEYMAP_H__

// ----------------------------------------------------------------------------------
/* Keyboard keycode map [0x00-0xff] (NR_KEYS = 256) */
#define RTD_KEY_BASE                    (0x100)
// ----------------------------------------------------------------------------------
// MCE Keycode Map : 0x100 + MCE HID scan code [0x100-0x16f] (KEY_MAX = 0x1ff)
#define RTD_MCE_KEY_HOME                    (RTD_KEY_BASE + 0x00d)                 
#define RTD_MCE_KEY_DVD_MENU                (RTD_KEY_BASE + 0x024) 
#define RTD_MCE_KEY_TV                      (RTD_KEY_BASE + 0x025) 
#define RTD_MCE_KEY_ZOOM                    (RTD_KEY_BASE + 0x027) 
#define RTD_MCE_KEY_EJECTCD                 (RTD_KEY_BASE + 0x028) 
#define RTD_MCE_KEY_CLOSED_CAPTIONING       (RTD_KEY_BASE + 0x02b) 
#define RTD_MCE_KEY_AUDIO                   (RTD_KEY_BASE + 0x031) 
#define RTD_MCE_KEY_TEXT                    (RTD_KEY_BASE + 0x032) 
#define RTD_MCE_KEY_CHANNEL                 (RTD_KEY_BASE + 0x033) 
#define RTD_MCE_KEY_DVD_TOP_MENU            (RTD_KEY_BASE + 0x043) 
#define RTD_MCE_KEY_REC_TV                  (RTD_KEY_BASE + 0x048) 
#define RTD_MCE_KEY_DVD_ANGLE               (RTD_KEY_BASE + 0x04b)
#define RTD_MCE_KEY_DVD_AUDIO               (RTD_KEY_BASE + 0x04c)
#define RTD_MCE_KEY_DVD_SUBTITLE            (RTD_KEY_BASE + 0x04d)       
#define RTD_MCE_KEY_TELETEXT_ONOFF          (RTD_KEY_BASE + 0x05a)
#define RTD_MCE_KEY_TELETEXT_RED            (RTD_KEY_BASE + 0x05b)
#define RTD_MCE_KEY_TELETEXT_GREEN          (RTD_KEY_BASE + 0x05c)
#define RTD_MCE_KEY_TELETEXT_YELLOW         (RTD_KEY_BASE + 0x05d)
#define RTD_MCE_KEY_TELETEXT_BLUE           (RTD_KEY_BASE + 0x05e)

// ----------------------------------------------------------------------------------
// Consumer Keycode Map : 0x170 + MCE HID scan code [0x170-0x1fe] (KEY_MAX = 0x1ff)
#define RTD_CONSUMER_KEY_BASE               (RTD_KEY_BASE + 0x70)

#define RTD_CONSUMER_KEY_GUIDE              (RTD_KEY_BASE + 0x8D)

#define RTD_CONSUMER_KEY_HELP               (RTD_KEY_BASE + 0x95)
#define RTD_CONSUMER_KEY_CHAN_PAGE_UP       (RTD_KEY_BASE + 0x9C)
#define RTD_CONSUMER_KEY_CHAN_PAGE_DOWN     (RTD_KEY_BASE + 0x9D)

#define RTD_CONSUMER_KEY_PLAY               (RTD_KEY_BASE + 0xB0)
#define RTD_CONSUMER_KEY_PAUSE              (RTD_KEY_BASE + 0xB1)
#define RTD_CONSUMER_KEY_RECORD             (RTD_KEY_BASE + 0xB2)
#define RTD_CONSUMER_KEY_FAST_FORWARD       (RTD_KEY_BASE + 0xB3)
#define RTD_CONSUMER_KEY_REWIND             (RTD_KEY_BASE + 0xB4)
#define RTD_CONSUMER_KEY_SKIP_FORWARD       (RTD_KEY_BASE + 0xB5)
#define RTD_CONSUMER_KEY_SKIP_BACK          (RTD_KEY_BASE + 0xB6)
#define RTD_CONSUMER_KEY_STOP               (RTD_KEY_BASE + 0xB7)
#define RTD_CONSUMER_KEY_EJECT              (RTD_KEY_BASE + 0xB8)
#define RTD_CONSUMER_KEY_REPEAT             (RTD_KEY_BASE + 0xBC)

#define RTD_CONSUMER_KEY_PLAY_PAUSE_COMBO   (RTD_KEY_BASE + 0xCD)
#define RTD_CONSUMER_KEY_PLAY_SKIP_COMBO    (RTD_KEY_BASE + 0xCE)

#define RTD_CONSUMER_KEY_VOLUME             (RTD_KEY_BASE + 0xE0)
#define RTD_CONSUMER_KEY_MUTE               (RTD_KEY_BASE + 0xE2)
#define RTD_CONSUMER_KEY_BASS_BOOST         (RTD_KEY_BASE + 0xE5)
#define RTD_CONSUMER_KEY_LOUDNESS           (RTD_KEY_BASE + 0xE7)
#define RTD_CONSUMER_KEY_VOLUME_UP          (RTD_KEY_BASE + 0xE9)
#define RTD_CONSUMER_KEY_VOLUME_DOWN        (RTD_KEY_BASE + 0xEA)

// Virtual
#define RTD_CONSUMER_KEY_BASS_UP            (RTD_KEY_BASE + 0xEF)
#define RTD_CONSUMER_KEY_BASS_DOWN          (RTD_KEY_BASE + 0xF0)
#define RTD_CONSUMER_KEY_TREBLE_UP          (RTD_KEY_BASE + 0xF1)
#define RTD_CONSUMER_KEY_TREBLE_DOWN        (RTD_KEY_BASE + 0xF2)
#define RTD_CONSUMER_KEY_MEDIA_SELECT       (RTD_KEY_BASE + 0xF3)

#define RTD_CONSUMER_KEY_MAIL               (RTD_KEY_BASE + 0xF4)
#define RTD_CONSUMER_KEY_CALCULATOR         (RTD_KEY_BASE + 0xF5)
#define RTD_CONSUMER_KEY_MY_COMPUTER        (RTD_KEY_BASE + 0xF6)
#define RTD_CONSUMER_KEY_MORE_INFO          (RTD_KEY_BASE + 0xF7)
#define RTD_CONSUMER_KEY_WWW_SEARCH         (RTD_KEY_BASE + 0xF8)
#define RTD_CONSUMER_KEY_WWW_HOME           (RTD_KEY_BASE + 0xF9)
#define RTD_CONSUMER_KEY_WWW_BACK           (RTD_KEY_BASE + 0xFA)

#define RTD_CONSUMER_KEY_WWW_FORWARD        (RTD_KEY_BASE + 0xFB)
#define RTD_CONSUMER_KEY_WWW_STOP           (RTD_KEY_BASE + 0xFC)
#define RTD_CONSUMER_KEY_WWW_REFRESH        (RTD_KEY_BASE + 0xFD)
#define RTD_CONSUMER_KEY_WWW_FAVORITES      (RTD_KEY_BASE + 0xFE)

// ----------------------------------------------------------------------------------
// Microsoft USB HID to PS/2 Scan Code Translation Table (E8-FFFF RESERVED)
#define RTD_GENDESK_KEY_BASE                (0xE8)
#define RTD_GENDESK_KEY_SYSTEM_POWER        (0xFD)
#define RTD_GENDESK_KEY_SYSTEM_SLEEP        (0xFE)
#define RTD_GENDESK_KEY_SYSTEM_WAKE         (0xFF)

#endif // __RTD_HID_KEYMAP_H__

