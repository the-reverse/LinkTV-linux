/**
 * \file libmtp.c
 *
 * Copyright (C) 2005-2009 Linus Walleij <triad@df.lth.se>
 * Copyright (C) 2005-2008 Richard A. Low <richard@wentnet.com>
 * Copyright (C) 2007 Ted Bullock <tbullock@canada.com>
 * Copyright (C) 2007 Tero Saarni <tero.saarni@gmail.com>
 * Copyright (C) 2008 Florent Mertens <flomertens@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * This file provides an interface "glue" to the underlying
 * PTP implementation from libgphoto2. It uses some local
 * code to convert from/to UTF-8 (stored in unicode.c/.h)
 * and some small utility functions, mainly for debugging
 * (stored in util.c/.h).
 *
 * The three PTP files (ptp.c, ptp.h and ptp-pack.c) are
 * plain copied from the libhphoto2 codebase.
 *
 * The files libusb-glue.c/.h are just what they say: an
 * interface to libusb for the actual, physical USB traffic.
 */

#include <linux/config.h>

// #include "config.h"
#include <linux/mtp/libmtp.h>
#include "unicode.h"
#include <linux/mtp/ptp.h>
#include <linux/mtp/usb.h>

#include "playlist-spl.h"
#include "util.h"



/**
 * Global debug level
 * We use a flag system to enable a part of logs.
 * To choose a particular flag, you have to use LIBMTP_DEBUG env variable.
 * Indeed, the option '-d' enables all logs.
 *  0x00 [0000 0000] : no debug (default)
 *  0x01 [0000 0001] : PTP debug
 *  0x02 [0000 0010] : Playlist debug
 *  0x04 [0000 0100] : USB debug
 *  0x08 [0000 1000] : USB data debug
 */
int LIBMTP_debug = LIBMTP_DEBUG_NONE; // LIBMTP_DEBUG_USB;


/*
 * This is a mapping between libmtp internal MTP filetypes and
 * the libgphoto2/PTP equivalent defines. We need this because
 * otherwise the libmtp.h device has to be dependent on ptp.h
 * to be installed too, and we don't want that.
 */
//typedef struct filemap_struct filemap_t;
typedef struct filemap_struct
{
    char *description; /**< Text description for the file type */
    LIBMTP_filetype_t id; /**< LIBMTP internal type for the file type */
    uint16_t ptp_id; /**< PTP ID for the filetype */
    struct filemap_struct *next;
} filemap_t;

/*
 * This is a mapping between libmtp internal MTP properties and
 * the libgphoto2/PTP equivalent defines. We need this because
 * otherwise the libmtp.h device has to be dependent on ptp.h
 * to be installed too, and we don't want that.
 */
typedef struct propertymap_struct
{
    char *description; /**< Text description for the property */
    LIBMTP_property_t id; /**< LIBMTP internal type for the property */
    uint16_t ptp_id; /**< PTP ID for the property */
    struct propertymap_struct *next;
} propertymap_t;

// Global variables
// This holds the global filetype mapping table
static filemap_t *filemap = NULL;
// This holds the global property mapping table
static propertymap_t *propertymap = NULL;

/*
 * Forward declarations of local (static) functions.
 */
static int register_filetype(char const *const description, LIBMTP_filetype_t
                             const id, uint16_t const ptp_id);
static void init_filemap(void);
static int register_property(char const *const description, LIBMTP_property_t
                             const id, uint16_t const ptp_id);
static void init_propertymap(void);
static void add_error_to_errorstack(LIBMTP_mtpdevice_t *device,
                                    LIBMTP_error_number_t errornumber, char
                                    const *const error_text);
static void add_ptp_error_to_errorstack(LIBMTP_mtpdevice_t *device, uint16_t
                                        ptp_error, char const *const error_text)
                                        ;
/*                                        
static void flush_handles(LIBMTP_mtpdevice_t *device);
static void get_handles_recursively(LIBMTP_mtpdevice_t *device,
                                    PTPParams*params, uint32_t storageid,
                                    uint32_t parent);
*/                                    
static void free_storage_list(LIBMTP_mtpdevice_t *device);
static int sort_storage_by(LIBMTP_mtpdevice_t *device, int const sortby);
static uint32_t get_writeable_storageid(LIBMTP_mtpdevice_t *device, uint64_t
                                        fitsize);
static int get_storage_freespace(LIBMTP_mtpdevice_t *device,
                                 LIBMTP_devicestorage_t *storage,
                                 uint64_t*freespace);
static int check_if_file_fits(LIBMTP_mtpdevice_t *device,
                              LIBMTP_devicestorage_t *storage, uint64_t const
                              filesize);
static uint16_t map_libmtp_type_to_ptp_type(LIBMTP_filetype_t intype);
static LIBMTP_filetype_t map_ptp_type_to_libmtp_type(uint16_t intype);
static uint16_t map_libmtp_property_to_ptp_property(LIBMTP_property_t
    inproperty);
static LIBMTP_property_t map_ptp_property_to_libmtp_property(uint16_t intype);
/*
static int get_device_unicode_property(LIBMTP_mtpdevice_t *device,
char **unicstring, uint16_t property);
 */

/*				       
static uint16_t adjust_u16(uint16_t val, PTPObjectPropDesc *opd);
static uint32_t adjust_u32(uint32_t val, PTPObjectPropDesc *opd);
 */
// static char *get_iso8601_stamp(void);
static char *get_string_from_object(LIBMTP_mtpdevice_t *device, uint32_t const
                                    object_id, uint16_t const attribute_id);
static uint64_t get_u64_from_object(LIBMTP_mtpdevice_t *device, uint32_t const
                                    object_id, uint16_t const attribute_id,
                                    uint64_t const value_default);
static uint32_t get_u32_from_object(LIBMTP_mtpdevice_t *device, uint32_t const
                                    object_id, uint16_t const attribute_id,
                                    uint32_t const value_default);
static uint16_t get_u16_from_object(LIBMTP_mtpdevice_t *device, uint32_t const
                                    object_id, uint16_t const attribute_id,
                                    uint16_t const value_default);
static uint8_t get_u8_from_object(LIBMTP_mtpdevice_t *device, uint32_t const
                                  object_id, uint16_t const attribute_id,
                                  uint8_t const value_default);
static int set_object_string(LIBMTP_mtpdevice_t *device, uint32_t const
                             object_id, uint16_t const attribute_id, char
                             const*const string);
static int set_object_u32(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
                          uint16_t const attribute_id, uint32_t const value);
static int set_object_u16(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
                          uint16_t const attribute_id, uint16_t const value);
static int set_object_u8(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
                         uint16_t const attribute_id, uint8_t const value);                              
static LIBMTP_folder_t *get_subfolders_for_folder(LIBMTP_folder_t *list,
    uint32_t parent);

/*
static int create_new_abstract_list(LIBMTP_mtpdevice_t *device,
char const * const name,
char const * const artist,
char const * const composer,
char const * const genre,
uint32_t const parenthandle,
uint32_t const storageid,
uint16_t const objectformat,
char const * const suffix,
uint32_t * const newid,
uint32_t const * const tracks,
uint32_t const no_tracks);

static int update_abstract_list(LIBMTP_mtpdevice_t *device,
char const * const name,
char const * const artist,
char const * const composer,
char const * const genre,
uint32_t const objecthandle,
uint16_t const objectformat,
uint32_t const * const tracks,
uint32_t const no_tracks);

static int send_file_object_info(LIBMTP_mtpdevice_t *device, LIBMTP_file_t *filedata);
 */

static void add_object_to_cache(LIBMTP_mtpdevice_t *device, uint32_t object_id);
// static void update_metadata_cache(LIBMTP_mtpdevice_t *device, uint32_t object_id);
/*
static int set_object_filename(LIBMTP_mtpdevice_t *device,
uint32_t object_id,
uint16_t ptp_type,
const char **newname);
 */
/*
static char *generate_unique_filename(PTPParams* params, char const * const filename);
static int check_filename_exists(PTPParams* params, char const * const filename);
 */

/**
 * These are to wrap the get/put handlers to convert from the MTP types to PTP types
 * in a reliable way
 */
typedef struct _MTPDataHandler
{
    MTPDataGetFunc getfunc;
    MTPDataPutFunc putfunc;
    void *priv;
} MTPDataHandler;

// static uint16_t get_func_wrapper(PTPParams* params, void* priv, unsigned long wantlen, unsigned char *data, unsigned long *gotlen);
static uint16_t put_func_wrapper(PTPParams *params, void *priv, unsigned long
                                 sendlen, unsigned char *data, unsigned
                                 long*putlen);

/**
 * Checks if a filename ends with ".ogg". Used in various
 * situations when the device has no idea that it support
 * OGG but still does.
 *
 * @param name string to be checked.
 * @return 0 if this does not end with ogg, any other
 *           value means it does.
 */
/* 
static int has_ogg_extension(char *name)
{
    char *ptype;

    if (name == NULL)
    {
        return 0;
    }
    ptype = strrchr(name, '.');
    if (ptype == NULL)
    {
        return 0;
    }
    if (!strcasecmp(ptype, ".ogg"))
    {
        return 1;
    }
    return 0;
}
*/
/**
 * Checks if a filename ends with ".flac". Used in various
 * situations when the device has no idea that it support
 * FLAC but still does.
 *
 * @param name string to be checked.
 * @return 0 if this does not end with flac, any other
 *           value means it does.
 */
/* 
static int has_flac_extension(char *name)
{
    char *ptype;

    if (name == NULL)
    {
        return 0;
    }
    ptype = strrchr(name, '.');
    if (ptype == NULL)
    {
        return 0;
    }
    if (!strcasecmp(ptype, ".flac"))
    {
        return 1;
    }
    return 0;
}
*/


/**
 * Create a new file mapping entry
 * @return a newly allocated filemapping entry.
 */
static filemap_t *new_filemap_entry(void)
{
    filemap_t *filemap;

    filemap = (filemap_t*)kmalloc(sizeof(filemap_t), GFP_KERNEL);

    if (filemap != NULL)
    {
        filemap->description = NULL;
        filemap->id = LIBMTP_FILETYPE_UNKNOWN;
        filemap->ptp_id = PTP_OFC_Undefined;
        filemap->next = NULL;
    }

    return filemap;
}

/**
 * Register an MTP or PTP filetype for data retrieval
 *
 * @param description Text description of filetype
 * @param id libmtp internal filetype id
 * @param ptp_id PTP filetype id
 * @return 0 for success any other value means error.
 */
static int register_filetype(char const *const description, LIBMTP_filetype_t
                             const id, uint16_t const ptp_id)
{
    filemap_t *new = NULL,  *_current;

    // Has this LIBMTP filetype been registered before ?
    _current = filemap;
    while (_current != NULL)
    {
        if (_current->id == id)
        {
            break;
        }
        _current = _current->next;
    }

    // Create the entry
    if (_current == NULL)
    {
        new = new_filemap_entry();
        if (new == NULL)
        {
            return 1;
        }

        new->id = id;
        if (description != NULL)
        {
            new->description = strdup((char*)description);
        }
        new->ptp_id = ptp_id;

        // Add the entry to the list
        if (filemap == NULL)
        {
            filemap = new;
        }
        else
        {
            _current = filemap;
            while (_current->next != NULL)
            {
                _current = _current->next;
            }
            _current->next = new;
        }
        // Update the existing entry
    }
    else
    {
        if (_current->description != NULL)
        {
            kfree(_current->description);
        }
        _current->description = NULL;
        if (description != NULL)
        {
            _current->description = strdup((char*)description);
        }
        _current->ptp_id = ptp_id;
    }

    return 0;
}

/**************************************************************************
 *                                                                        *
 **************************************************************************/

static void init_filemap(void)
{
    register_filetype("MediaCard", LIBMTP_FILETYPE_MEDIACARD,
                      PTP_OFC_MTP_MediaCard);
    register_filetype("RIFF WAVE file", LIBMTP_FILETYPE_WAV, PTP_OFC_WAV);
    register_filetype("ISO MPEG-1 Audio Layer 3", LIBMTP_FILETYPE_MP3,
                      PTP_OFC_MP3);
    register_filetype("ISO MPEG-1 Audio Layer 2", LIBMTP_FILETYPE_MP2,
                      PTP_OFC_MTP_MP2);
    register_filetype("Microsoft Windows Media Audio", LIBMTP_FILETYPE_WMA,
                      PTP_OFC_MTP_WMA);
    register_filetype("Ogg container format", LIBMTP_FILETYPE_OGG,
                      PTP_OFC_MTP_OGG);
    register_filetype("Free Lossless Audio Codec (FLAC)", LIBMTP_FILETYPE_FLAC,
                      PTP_OFC_MTP_FLAC);
    register_filetype("Advanced Audio Coding (AAC)/MPEG-2 Part 7/MPEG-4 Part 3",
                      LIBMTP_FILETYPE_AAC, PTP_OFC_MTP_AAC);
    register_filetype("MPEG-4 Part 14 Container Format (Audio Emphasis)",
                      LIBMTP_FILETYPE_M4A, PTP_OFC_MTP_M4A);
    register_filetype("MPEG-4 Part 14 Container Format (Audio+Video Emphasis)",
                      LIBMTP_FILETYPE_MP4, PTP_OFC_MTP_MP4);
    register_filetype("Audible.com Audio Codec", LIBMTP_FILETYPE_AUDIBLE,
                      PTP_OFC_MTP_AudibleCodec);
    register_filetype("Undefined audio file", LIBMTP_FILETYPE_UNDEF_AUDIO,
                      PTP_OFC_MTP_UndefinedAudio);
    register_filetype("Microsoft Windows Media Video", LIBMTP_FILETYPE_WMV,
                      PTP_OFC_MTP_WMV);
    register_filetype("Audio Video Interleave", LIBMTP_FILETYPE_AVI,
                      PTP_OFC_AVI);
    register_filetype("MPEG video stream", LIBMTP_FILETYPE_MPEG, PTP_OFC_MPEG);
    register_filetype("Microsoft Advanced Systems Format", LIBMTP_FILETYPE_ASF,
                      PTP_OFC_ASF);
    register_filetype("Apple Quicktime container format", LIBMTP_FILETYPE_QT,
                      PTP_OFC_QT);
    register_filetype("Undefined video file", LIBMTP_FILETYPE_UNDEF_VIDEO,
                      PTP_OFC_MTP_UndefinedVideo);
    register_filetype("JPEG file", LIBMTP_FILETYPE_JPEG, PTP_OFC_EXIF_JPEG);
    register_filetype("JP2 file", LIBMTP_FILETYPE_JP2, PTP_OFC_JP2);
    register_filetype("JPX file", LIBMTP_FILETYPE_JPX, PTP_OFC_JPX);
    register_filetype("JFIF file", LIBMTP_FILETYPE_JFIF, PTP_OFC_JFIF);
    register_filetype("TIFF bitmap file", LIBMTP_FILETYPE_TIFF, PTP_OFC_TIFF);
    register_filetype("BMP bitmap file", LIBMTP_FILETYPE_BMP, PTP_OFC_BMP);
    register_filetype("GIF bitmap file", LIBMTP_FILETYPE_GIF, PTP_OFC_GIF);
    register_filetype("PICT bitmap file", LIBMTP_FILETYPE_PICT, PTP_OFC_PICT);
    register_filetype("Portable Network Graphics", LIBMTP_FILETYPE_PNG,
                      PTP_OFC_PNG);
    register_filetype("Microsoft Windows Image Format",
                      LIBMTP_FILETYPE_WINDOWSIMAGEFORMAT,
                      PTP_OFC_MTP_WindowsImageFormat);
    register_filetype("VCalendar version 1", LIBMTP_FILETYPE_VCALENDAR1,
                      PTP_OFC_MTP_vCalendar1);
    register_filetype("VCalendar version 2", LIBMTP_FILETYPE_VCALENDAR2,
                      PTP_OFC_MTP_vCalendar2);
    register_filetype("VCard version 2", LIBMTP_FILETYPE_VCARD2,
                      PTP_OFC_MTP_vCard2);
    register_filetype("VCard version 3", LIBMTP_FILETYPE_VCARD3,
                      PTP_OFC_MTP_vCard3);
    register_filetype("Undefined Windows executable file",
                      LIBMTP_FILETYPE_WINEXEC,
                      PTP_OFC_MTP_UndefinedWindowsExecutable);
    register_filetype("Text file", LIBMTP_FILETYPE_TEXT, PTP_OFC_Text);
    register_filetype("HTML file", LIBMTP_FILETYPE_HTML, PTP_OFC_HTML);
    register_filetype("XML file", LIBMTP_FILETYPE_XML, PTP_OFC_MTP_XMLDocument);
    register_filetype("DOC file", LIBMTP_FILETYPE_DOC,
                      PTP_OFC_MTP_MSWordDocument);
    register_filetype("XLS file", LIBMTP_FILETYPE_XLS,
                      PTP_OFC_MTP_MSExcelSpreadsheetXLS);
    register_filetype("PPT file", LIBMTP_FILETYPE_PPT,
                      PTP_OFC_MTP_MSPowerpointPresentationPPT);
    register_filetype("MHT file", LIBMTP_FILETYPE_MHT,
                      PTP_OFC_MTP_MHTCompiledHTMLDocument);
    register_filetype("Firmware file", LIBMTP_FILETYPE_FIRMWARE,
                      PTP_OFC_MTP_Firmware);
    register_filetype("Abstract Album file", LIBMTP_FILETYPE_ALBUM,
                      PTP_OFC_MTP_AbstractAudioAlbum);
    register_filetype("Abstract Playlist file", LIBMTP_FILETYPE_PLAYLIST,
                      PTP_OFC_MTP_AbstractAudioVideoPlaylist);
    register_filetype("Undefined filetype", LIBMTP_FILETYPE_UNKNOWN,
                      PTP_OFC_Undefined);
}

/**
 * Returns the PTP filetype that maps to a certain libmtp internal file type.
 * @param intype the MTP library interface type
 * @return the PTP (libgphoto2) interface type
 */
static uint16_t map_libmtp_type_to_ptp_type(LIBMTP_filetype_t intype)
{
    filemap_t *_current;

    _current = filemap;

    while (_current != NULL)
    {
        if (_current->id == intype)
        {
            return _current->ptp_id;
        }
        _current = _current->next;
    }
    // printk("map_libmtp_type_to_ptp_type: unknown filetype.\n");
    return PTP_OFC_Undefined;
}


/**
 * Returns the MTP internal interface type that maps to a certain ptp
 * interface type.
 * @param intype the PTP (libgphoto2) interface type
 * @return the MTP library interface type
 */
static LIBMTP_filetype_t map_ptp_type_to_libmtp_type(uint16_t intype)
{
    filemap_t *_current;

    _current = filemap;

    while (_current != NULL)
    {
        if (_current->ptp_id == intype)
        {
            return _current->id;
        }
        _current = _current->next;
    }
    // printk("map_ptp_type_to_libmtp_type: unknown filetype.\n");
    return LIBMTP_FILETYPE_UNKNOWN;
}

/**
 * Create a new property mapping entry
 * @return a newly allocated propertymapping entry.
 */
static propertymap_t *new_propertymap_entry(void)
{
    propertymap_t *propertymap;

    propertymap = (propertymap_t*)kmalloc(sizeof(propertymap_t), GFP_KERNEL);

    if (propertymap != NULL)
    {
        propertymap->description = NULL;
        propertymap->id = LIBMTP_PROPERTY_UNKNOWN;
        propertymap->ptp_id = 0;
        propertymap->next = NULL;
    }

    return propertymap;
}

/**
 * Register an MTP or PTP property for data retrieval
 *
 * @param description Text description of property
 * @param id libmtp internal property id
 * @param ptp_id PTP property id
 * @return 0 for success any other value means error.
 */
static int register_property(char const *const description, LIBMTP_property_t
                             const id, uint16_t const ptp_id)
{
    propertymap_t *new = NULL,  *_current;

    // Has this LIBMTP propety been registered before ?
    _current = propertymap;
    while (_current != NULL)
    {
        if (_current->id == id)
        {
            break;
        }
        _current = _current->next;
    }

    // Create the entry
    if (_current == NULL)
    {
        new = new_propertymap_entry();
        if (new == NULL)
        {
            return 1;
        }

        new->id = id;
        if (description != NULL)
        {
            new->description = strdup((char*)description);
        }
        new->ptp_id = ptp_id;

        // Add the entry to the list
        if (propertymap == NULL)
        {
            propertymap = new;
        }
        else
        {
            _current = propertymap;
            while (_current->next != NULL)
            {
                _current = _current->next;
            }
            _current->next = new;
        }
        // Update the existing entry
    }
    else
    {
        if (_current->description != NULL)
        {
            kfree(_current->description);
        }
        _current->description = NULL;
        if (description != NULL)
        {
            _current->description = strdup((char*)description);
        }
        _current->ptp_id = ptp_id;
    }

    return 0;
}

/**************************************************************************
 *                                                                        *
 **************************************************************************/

static void init_propertymap(void)
{
    register_property("Storage ID", LIBMTP_PROPERTY_StorageID,
                      PTP_OPC_StorageID);
    register_property("Object Format", LIBMTP_PROPERTY_ObjectFormat,
                      PTP_OPC_ObjectFormat);
    register_property("Protection Status", LIBMTP_PROPERTY_ProtectionStatus,
                      PTP_OPC_ProtectionStatus);
    register_property("Object Size", LIBMTP_PROPERTY_ObjectSize,
                      PTP_OPC_ObjectSize);
    register_property("Association Type", LIBMTP_PROPERTY_AssociationType,
                      PTP_OPC_AssociationType);
    register_property("Association Desc", LIBMTP_PROPERTY_AssociationDesc,
                      PTP_OPC_AssociationDesc);
    register_property("Object File Name", LIBMTP_PROPERTY_ObjectFileName,
                      PTP_OPC_ObjectFileName);
    register_property("Date Created", LIBMTP_PROPERTY_DateCreated,
                      PTP_OPC_DateCreated);
    register_property("Date Modified", LIBMTP_PROPERTY_DateModified,
                      PTP_OPC_DateModified);
    register_property("Keywords", LIBMTP_PROPERTY_Keywords, PTP_OPC_Keywords);
    register_property("Parent Object", LIBMTP_PROPERTY_ParentObject,
                      PTP_OPC_ParentObject);
    register_property("Allowed Folder Contents",
                      LIBMTP_PROPERTY_AllowedFolderContents,
                      PTP_OPC_AllowedFolderContents);
    register_property("Hidden", LIBMTP_PROPERTY_Hidden, PTP_OPC_Hidden);
    register_property("System Object", LIBMTP_PROPERTY_SystemObject,
                      PTP_OPC_SystemObject);
    register_property("Persistant Unique Object Identifier",
                      LIBMTP_PROPERTY_PersistantUniqueObjectIdentifier,
                      PTP_OPC_PersistantUniqueObjectIdentifier);
    register_property("Sync ID", LIBMTP_PROPERTY_SyncID, PTP_OPC_SyncID);
    register_property("Property Bag", LIBMTP_PROPERTY_PropertyBag,
                      PTP_OPC_PropertyBag);
    register_property("Name", LIBMTP_PROPERTY_Name, PTP_OPC_Name);
    register_property("Created By", LIBMTP_PROPERTY_CreatedBy,
                      PTP_OPC_CreatedBy);
    register_property("Artist", LIBMTP_PROPERTY_Artist, PTP_OPC_Artist);
    register_property("Date Authored", LIBMTP_PROPERTY_DateAuthored,
                      PTP_OPC_DateAuthored);
    register_property("Description", LIBMTP_PROPERTY_Description,
                      PTP_OPC_Description);
    register_property("URL Reference", LIBMTP_PROPERTY_URLReference,
                      PTP_OPC_URLReference);
    register_property("Language Locale", LIBMTP_PROPERTY_LanguageLocale,
                      PTP_OPC_LanguageLocale);
    register_property("Copyright Information",
                      LIBMTP_PROPERTY_CopyrightInformation,
                      PTP_OPC_CopyrightInformation);
    register_property("Source", LIBMTP_PROPERTY_Source, PTP_OPC_Source);
    register_property("Origin Location", LIBMTP_PROPERTY_OriginLocation,
                      PTP_OPC_OriginLocation);
    register_property("Date Added", LIBMTP_PROPERTY_DateAdded,
                      PTP_OPC_DateAdded);
    register_property("Non Consumable", LIBMTP_PROPERTY_NonConsumable,
                      PTP_OPC_NonConsumable);
    register_property("Corrupt Or Unplayable",
                      LIBMTP_PROPERTY_CorruptOrUnplayable,
                      PTP_OPC_CorruptOrUnplayable);
    register_property("Producer Serial Number",
                      LIBMTP_PROPERTY_ProducerSerialNumber,
                      PTP_OPC_ProducerSerialNumber);
    register_property("Representative Sample Format",
                      LIBMTP_PROPERTY_RepresentativeSampleFormat,
                      PTP_OPC_RepresentativeSampleFormat);
    register_property("Representative Sample Sise",
                      LIBMTP_PROPERTY_RepresentativeSampleSize,
                      PTP_OPC_RepresentativeSampleSize);
    register_property("Representative Sample Height",
                      LIBMTP_PROPERTY_RepresentativeSampleHeight,
                      PTP_OPC_RepresentativeSampleHeight);
    register_property("Representative Sample Width",
                      LIBMTP_PROPERTY_RepresentativeSampleWidth,
                      PTP_OPC_RepresentativeSampleWidth);
    register_property("Representative Sample Duration",
                      LIBMTP_PROPERTY_RepresentativeSampleDuration,
                      PTP_OPC_RepresentativeSampleDuration);
    register_property("Representative Sample Data",
                      LIBMTP_PROPERTY_RepresentativeSampleData,
                      PTP_OPC_RepresentativeSampleData);
    register_property("Width", LIBMTP_PROPERTY_Width, PTP_OPC_Width);
    register_property("Height", LIBMTP_PROPERTY_Height, PTP_OPC_Height);
    register_property("Duration", LIBMTP_PROPERTY_Duration, PTP_OPC_Duration);
    register_property("Rating", LIBMTP_PROPERTY_Rating, PTP_OPC_Rating);
    register_property("Track", LIBMTP_PROPERTY_Track, PTP_OPC_Track);
    register_property("Genre", LIBMTP_PROPERTY_Genre, PTP_OPC_Genre);
    register_property("Credits", LIBMTP_PROPERTY_Credits, PTP_OPC_Credits);
    register_property("Lyrics", LIBMTP_PROPERTY_Lyrics, PTP_OPC_Lyrics);
    register_property("Subscription Content ID",
                      LIBMTP_PROPERTY_SubscriptionContentID,
                      PTP_OPC_SubscriptionContentID);
    register_property("Produced By", LIBMTP_PROPERTY_ProducedBy,
                      PTP_OPC_ProducedBy);
    register_property("Use Count", LIBMTP_PROPERTY_UseCount, PTP_OPC_UseCount);
    register_property("Skip Count", LIBMTP_PROPERTY_SkipCount,
                      PTP_OPC_SkipCount);
    register_property("Last Accessed", LIBMTP_PROPERTY_LastAccessed,
                      PTP_OPC_LastAccessed);
    register_property("Parental Rating", LIBMTP_PROPERTY_ParentalRating,
                      PTP_OPC_ParentalRating);
    register_property("Meta Genre", LIBMTP_PROPERTY_MetaGenre,
                      PTP_OPC_MetaGenre);
    register_property("Composer", LIBMTP_PROPERTY_Composer, PTP_OPC_Composer);
    register_property("Effective Rating", LIBMTP_PROPERTY_EffectiveRating,
                      PTP_OPC_EffectiveRating);
    register_property("Subtitle", LIBMTP_PROPERTY_Subtitle, PTP_OPC_Subtitle);
    register_property("Original Release Date",
                      LIBMTP_PROPERTY_OriginalReleaseDate,
                      PTP_OPC_OriginalReleaseDate);
    register_property("Album Name", LIBMTP_PROPERTY_AlbumName,
                      PTP_OPC_AlbumName);
    register_property("Album Artist", LIBMTP_PROPERTY_AlbumArtist,
                      PTP_OPC_AlbumArtist);
    register_property("Mood", LIBMTP_PROPERTY_Mood, PTP_OPC_Mood);
    register_property("DRM Status", LIBMTP_PROPERTY_DRMStatus,
                      PTP_OPC_DRMStatus);
    register_property("Sub Description", LIBMTP_PROPERTY_SubDescription,
                      PTP_OPC_SubDescription);
    register_property("Is Cropped", LIBMTP_PROPERTY_IsCropped,
                      PTP_OPC_IsCropped);
    register_property("Is Color Corrected", LIBMTP_PROPERTY_IsColorCorrected,
                      PTP_OPC_IsColorCorrected);
    register_property("Image Bit Depth", LIBMTP_PROPERTY_ImageBitDepth,
                      PTP_OPC_ImageBitDepth);
    register_property("f Number", LIBMTP_PROPERTY_Fnumber, PTP_OPC_Fnumber);
    register_property("Exposure Time", LIBMTP_PROPERTY_ExposureTime,
                      PTP_OPC_ExposureTime);
    register_property("Exposure Index", LIBMTP_PROPERTY_ExposureIndex,
                      PTP_OPC_ExposureIndex);
    register_property("Display Name", LIBMTP_PROPERTY_DisplayName,
                      PTP_OPC_DisplayName);
    register_property("Body Text", LIBMTP_PROPERTY_BodyText, PTP_OPC_BodyText);
    register_property("Subject", LIBMTP_PROPERTY_Subject, PTP_OPC_Subject);
    register_property("Priority", LIBMTP_PROPERTY_Priority, PTP_OPC_Priority);
    register_property("Given Name", LIBMTP_PROPERTY_GivenName,
                      PTP_OPC_GivenName);
    register_property("Middle Names", LIBMTP_PROPERTY_MiddleNames,
                      PTP_OPC_MiddleNames);
    register_property("Family Name", LIBMTP_PROPERTY_FamilyName,
                      PTP_OPC_FamilyName);
    register_property("Prefix", LIBMTP_PROPERTY_Prefix, PTP_OPC_Prefix);
    register_property("Suffix", LIBMTP_PROPERTY_Suffix, PTP_OPC_Suffix);
    register_property("Phonetic Given Name", LIBMTP_PROPERTY_PhoneticGivenName,
                      PTP_OPC_PhoneticGivenName);
    register_property("Phonetic Family Name",
                      LIBMTP_PROPERTY_PhoneticFamilyName,
                      PTP_OPC_PhoneticFamilyName);
    register_property("Email: Primary", LIBMTP_PROPERTY_EmailPrimary,
                      PTP_OPC_EmailPrimary);
    register_property("Email: Personal 1", LIBMTP_PROPERTY_EmailPersonal1,
                      PTP_OPC_EmailPersonal1);
    register_property("Email: Personal 2", LIBMTP_PROPERTY_EmailPersonal2,
                      PTP_OPC_EmailPersonal2);
    register_property("Email: Business 1", LIBMTP_PROPERTY_EmailBusiness1,
                      PTP_OPC_EmailBusiness1);
    register_property("Email: Business 2", LIBMTP_PROPERTY_EmailBusiness2,
                      PTP_OPC_EmailBusiness2);
    register_property("Email: Others", LIBMTP_PROPERTY_EmailOthers,
                      PTP_OPC_EmailOthers);
    register_property("Phone Number: Primary",
                      LIBMTP_PROPERTY_PhoneNumberPrimary,
                      PTP_OPC_PhoneNumberPrimary);
    register_property("Phone Number: Personal",
                      LIBMTP_PROPERTY_PhoneNumberPersonal,
                      PTP_OPC_PhoneNumberPersonal);
    register_property("Phone Number: Personal 2",
                      LIBMTP_PROPERTY_PhoneNumberPersonal2,
                      PTP_OPC_PhoneNumberPersonal2);
    register_property("Phone Number: Business",
                      LIBMTP_PROPERTY_PhoneNumberBusiness,
                      PTP_OPC_PhoneNumberBusiness);
    register_property("Phone Number: Business 2",
                      LIBMTP_PROPERTY_PhoneNumberBusiness2,
                      PTP_OPC_PhoneNumberBusiness2);
    register_property("Phone Number: Mobile", LIBMTP_PROPERTY_PhoneNumberMobile,
                      PTP_OPC_PhoneNumberMobile);
    register_property("Phone Number: Mobile 2",
                      LIBMTP_PROPERTY_PhoneNumberMobile2,
                      PTP_OPC_PhoneNumberMobile2);
    register_property("Fax Number: Primary", LIBMTP_PROPERTY_FaxNumberPrimary,
                      PTP_OPC_FaxNumberPrimary);
    register_property("Fax Number: Personal", LIBMTP_PROPERTY_FaxNumberPersonal,
                      PTP_OPC_FaxNumberPersonal);
    register_property("Fax Number: Business", LIBMTP_PROPERTY_FaxNumberBusiness,
                      PTP_OPC_FaxNumberBusiness);
    register_property("Pager Number", LIBMTP_PROPERTY_PagerNumber,
                      PTP_OPC_PagerNumber);
    register_property("Phone Number: Others", LIBMTP_PROPERTY_PhoneNumberOthers,
                      PTP_OPC_PhoneNumberOthers);
    register_property("Primary Web Address", LIBMTP_PROPERTY_PrimaryWebAddress,
                      PTP_OPC_PrimaryWebAddress);
    register_property("Personal Web Address",
                      LIBMTP_PROPERTY_PersonalWebAddress,
                      PTP_OPC_PersonalWebAddress);
    register_property("Business Web Address",
                      LIBMTP_PROPERTY_BusinessWebAddress,
                      PTP_OPC_BusinessWebAddress);
    register_property("Instant Messenger Address 1",
                      LIBMTP_PROPERTY_InstantMessengerAddress,
                      PTP_OPC_InstantMessengerAddress);
    register_property("Instant Messenger Address 2",
                      LIBMTP_PROPERTY_InstantMessengerAddress2,
                      PTP_OPC_InstantMessengerAddress2);
    register_property("Instant Messenger Address 3",
                      LIBMTP_PROPERTY_InstantMessengerAddress3,
                      PTP_OPC_InstantMessengerAddress3);
    register_property("Postal Address: Personal: Full",
                      LIBMTP_PROPERTY_PostalAddressPersonalFull,
                      PTP_OPC_PostalAddressPersonalFull);
    register_property("Postal Address: Personal: Line 1",
                      LIBMTP_PROPERTY_PostalAddressPersonalFullLine1,
                      PTP_OPC_PostalAddressPersonalFullLine1);
    register_property("Postal Address: Personal: Line 2",
                      LIBMTP_PROPERTY_PostalAddressPersonalFullLine2,
                      PTP_OPC_PostalAddressPersonalFullLine2);
    register_property("Postal Address: Personal: City",
                      LIBMTP_PROPERTY_PostalAddressPersonalFullCity,
                      PTP_OPC_PostalAddressPersonalFullCity);
    register_property("Postal Address: Personal: Region",
                      LIBMTP_PROPERTY_PostalAddressPersonalFullRegion,
                      PTP_OPC_PostalAddressPersonalFullRegion);
    register_property("Postal Address: Personal: Postal Code",
                      LIBMTP_PROPERTY_PostalAddressPersonalFullPostalCode,
                      PTP_OPC_PostalAddressPersonalFullPostalCode);
    register_property("Postal Address: Personal: Country",
                      LIBMTP_PROPERTY_PostalAddressPersonalFullCountry,
                      PTP_OPC_PostalAddressPersonalFullCountry);
    register_property("Postal Address: Business: Full",
                      LIBMTP_PROPERTY_PostalAddressBusinessFull,
                      PTP_OPC_PostalAddressBusinessFull);
    register_property("Postal Address: Business: Line 1",
                      LIBMTP_PROPERTY_PostalAddressBusinessLine1,
                      PTP_OPC_PostalAddressBusinessLine1);
    register_property("Postal Address: Business: Line 2",
                      LIBMTP_PROPERTY_PostalAddressBusinessLine2,
                      PTP_OPC_PostalAddressBusinessLine2);
    register_property("Postal Address: Business: City",
                      LIBMTP_PROPERTY_PostalAddressBusinessCity,
                      PTP_OPC_PostalAddressBusinessCity);
    register_property("Postal Address: Business: Region",
                      LIBMTP_PROPERTY_PostalAddressBusinessRegion,
                      PTP_OPC_PostalAddressBusinessRegion);
    register_property("Postal Address: Business: Postal Code",
                      LIBMTP_PROPERTY_PostalAddressBusinessPostalCode,
                      PTP_OPC_PostalAddressBusinessPostalCode);
    register_property("Postal Address: Business: Country",
                      LIBMTP_PROPERTY_PostalAddressBusinessCountry,
                      PTP_OPC_PostalAddressBusinessCountry);
    register_property("Postal Address: Other: Full",
                      LIBMTP_PROPERTY_PostalAddressOtherFull,
                      PTP_OPC_PostalAddressOtherFull);
    register_property("Postal Address: Other: Line 1",
                      LIBMTP_PROPERTY_PostalAddressOtherLine1,
                      PTP_OPC_PostalAddressOtherLine1);
    register_property("Postal Address: Other: Line 2",
                      LIBMTP_PROPERTY_PostalAddressOtherLine2,
                      PTP_OPC_PostalAddressOtherLine2);
    register_property("Postal Address: Other: City",
                      LIBMTP_PROPERTY_PostalAddressOtherCity,
                      PTP_OPC_PostalAddressOtherCity);
    register_property("Postal Address: Other: Region",
                      LIBMTP_PROPERTY_PostalAddressOtherRegion,
                      PTP_OPC_PostalAddressOtherRegion);
    register_property("Postal Address: Other: Postal Code",
                      LIBMTP_PROPERTY_PostalAddressOtherPostalCode,
                      PTP_OPC_PostalAddressOtherPostalCode);
    register_property("Postal Address: Other: Counrtry",
                      LIBMTP_PROPERTY_PostalAddressOtherCountry,
                      PTP_OPC_PostalAddressOtherCountry);
    register_property("Organization Name", LIBMTP_PROPERTY_OrganizationName,
                      PTP_OPC_OrganizationName);
    register_property("Phonetic Organization Name",
                      LIBMTP_PROPERTY_PhoneticOrganizationName,
                      PTP_OPC_PhoneticOrganizationName);
    register_property("Role", LIBMTP_PROPERTY_Role, PTP_OPC_Role);
    register_property("Birthdate", LIBMTP_PROPERTY_Birthdate, PTP_OPC_Birthdate)
                      ;
    register_property("Message To", LIBMTP_PROPERTY_MessageTo,
                      PTP_OPC_MessageTo);
    register_property("Message CC", LIBMTP_PROPERTY_MessageCC,
                      PTP_OPC_MessageCC);
    register_property("Message BCC", LIBMTP_PROPERTY_MessageBCC,
                      PTP_OPC_MessageBCC);
    register_property("Message Read", LIBMTP_PROPERTY_MessageRead,
                      PTP_OPC_MessageRead);
    register_property("Message Received Time",
                      LIBMTP_PROPERTY_MessageReceivedTime,
                      PTP_OPC_MessageReceivedTime);
    register_property("Message Sender", LIBMTP_PROPERTY_MessageSender,
                      PTP_OPC_MessageSender);
    register_property("Activity Begin Time", LIBMTP_PROPERTY_ActivityBeginTime,
                      PTP_OPC_ActivityBeginTime);
    register_property("Activity End Time", LIBMTP_PROPERTY_ActivityEndTime,
                      PTP_OPC_ActivityEndTime);
    register_property("Activity Location", LIBMTP_PROPERTY_ActivityLocation,
                      PTP_OPC_ActivityLocation);
    register_property("Activity Required Attendees",
                      LIBMTP_PROPERTY_ActivityRequiredAttendees,
                      PTP_OPC_ActivityRequiredAttendees);
    register_property("Optional Attendees",
                      LIBMTP_PROPERTY_ActivityOptionalAttendees,
                      PTP_OPC_ActivityOptionalAttendees);
    register_property("Activity Resources", LIBMTP_PROPERTY_ActivityResources,
                      PTP_OPC_ActivityResources);
    register_property("Activity Accepted", LIBMTP_PROPERTY_ActivityAccepted,
                      PTP_OPC_ActivityAccepted);
    register_property("Owner", LIBMTP_PROPERTY_Owner, PTP_OPC_Owner);
    register_property("Editor", LIBMTP_PROPERTY_Editor, PTP_OPC_Editor);
    register_property("Webmaster", LIBMTP_PROPERTY_Webmaster, PTP_OPC_Webmaster)
                      ;
    register_property("URL Source", LIBMTP_PROPERTY_URLSource,
                      PTP_OPC_URLSource);
    register_property("URL Destination", LIBMTP_PROPERTY_URLDestination,
                      PTP_OPC_URLDestination);
    register_property("Time Bookmark", LIBMTP_PROPERTY_TimeBookmark,
                      PTP_OPC_TimeBookmark);
    register_property("Object Bookmark", LIBMTP_PROPERTY_ObjectBookmark,
                      PTP_OPC_ObjectBookmark);
    register_property("Byte Bookmark", LIBMTP_PROPERTY_ByteBookmark,
                      PTP_OPC_ByteBookmark);
    register_property("Last Build Date", LIBMTP_PROPERTY_LastBuildDate,
                      PTP_OPC_LastBuildDate);
    register_property("Time To Live", LIBMTP_PROPERTY_TimetoLive,
                      PTP_OPC_TimetoLive);
    register_property("Media GUID", LIBMTP_PROPERTY_MediaGUID,
                      PTP_OPC_MediaGUID);
    register_property("Total Bit Rate", LIBMTP_PROPERTY_TotalBitRate,
                      PTP_OPC_TotalBitRate);
    register_property("Bit Rate Type", LIBMTP_PROPERTY_BitRateType,
                      PTP_OPC_BitRateType);
    register_property("Sample Rate", LIBMTP_PROPERTY_SampleRate,
                      PTP_OPC_SampleRate);
    register_property("Number Of Channels", LIBMTP_PROPERTY_NumberOfChannels,
                      PTP_OPC_NumberOfChannels);
    register_property("Audio Bit Depth", LIBMTP_PROPERTY_AudioBitDepth,
                      PTP_OPC_AudioBitDepth);
    register_property("Scan Depth", LIBMTP_PROPERTY_ScanDepth,
                      PTP_OPC_ScanDepth);
    register_property("Audio WAVE Codec", LIBMTP_PROPERTY_AudioWAVECodec,
                      PTP_OPC_AudioWAVECodec);
    register_property("Audio Bit Rate", LIBMTP_PROPERTY_AudioBitRate,
                      PTP_OPC_AudioBitRate);
    register_property("Video Four CC Codec", LIBMTP_PROPERTY_VideoFourCCCodec,
                      PTP_OPC_VideoFourCCCodec);
    register_property("Video Bit Rate", LIBMTP_PROPERTY_VideoBitRate,
                      PTP_OPC_VideoBitRate);
    register_property("Frames Per Thousand Seconds",
                      LIBMTP_PROPERTY_FramesPerThousandSeconds,
                      PTP_OPC_FramesPerThousandSeconds);
    register_property("Key Frame Distance", LIBMTP_PROPERTY_KeyFrameDistance,
                      PTP_OPC_KeyFrameDistance);
    register_property("Buffer Size", LIBMTP_PROPERTY_BufferSize,
                      PTP_OPC_BufferSize);
    register_property("Encoding Quality", LIBMTP_PROPERTY_EncodingQuality,
                      PTP_OPC_EncodingQuality);
    register_property("Encoding Profile", LIBMTP_PROPERTY_EncodingProfile,
                      PTP_OPC_EncodingProfile);
    register_property("Buy flag", LIBMTP_PROPERTY_BuyFlag, PTP_OPC_BuyFlag);
    register_property("Unknown property", LIBMTP_PROPERTY_UNKNOWN, 0);
}

/**
 * Returns the PTP property that maps to a certain libmtp internal property type.
 * @param inproperty the MTP library interface property
 * @return the PTP (libgphoto2) property type
 */
static uint16_t map_libmtp_property_to_ptp_property(LIBMTP_property_t
    inproperty)
{
    propertymap_t *_current;

    _current = propertymap;

    while (_current != NULL)
    {
        if (_current->id == inproperty)
        {
            return _current->ptp_id;
        }
        _current = _current->next;
    }
    return 0;
}


/**
 * Returns the MTP internal interface property that maps to a certain ptp
 * interface property.
 * @param inproperty the PTP (libgphoto2) interface property
 * @return the MTP library interface property
 */
static LIBMTP_property_t map_ptp_property_to_libmtp_property(uint16_t
    inproperty)
{
    propertymap_t *_current;

    _current = propertymap;

    while (_current != NULL)
    {
        if (_current->ptp_id == inproperty)
        {
            return _current->id;
        }
        _current = _current->next;
    }
    // printk("map_ptp_type_to_libmtp_type: unknown filetype.\n");
    return LIBMTP_PROPERTY_UNKNOWN;
}


/**
 * Set the debug level.
 *
 * By default, the debug level is set to '0' (disable).
 */
void LIBMTP_Set_Debug(int level)
{
    if (LIBMTP_debug || level)
    {
        LIBMTP_ERROR("LIBMTP_Set_Debug: Setting debugging level to %d (%s)\n",
                     level, level ? "on" : "off");
    }

    LIBMTP_debug = level;
}


/**
 * Initialize the library. You are only supposed to call this
 * one, before using the library for the first time in a program.
 * Never re-initialize libmtp!
 *
 * The only thing this does at the moment is to initialise the
 * filetype mapping table.
 */
void LIBMTP_Init(void)
{
    /*  
    if (getenv("LIBMTP_DEBUG"))
    LIBMTP_Set_Debug(atoi(getenv("LIBMTP_DEBUG")));
     */

    init_filemap();
    init_propertymap();
    return ;
}


/**
 * This helper function returns a textual description for a libmtp
 * file type to be used in dialog boxes etc.
 * @param intype the libmtp internal filetype to get a description for.
 * @return a string representing the filetype, this must <b>NOT</b>
 *         be free():ed by the caller!
 */
char const *LIBMTP_Get_Filetype_Description(LIBMTP_filetype_t intype)
{
    filemap_t *_current;

    _current = filemap;

    while (_current != NULL)
    {
        if (_current->id == intype)
        {
            return _current->description;
        }
        _current = _current->next;
    }

    return "Unknown filetype";
}

/**
 * This helper function returns a textual description for a libmtp
 * property to be used in dialog boxes etc.
 * @param inproperty the libmtp internal property to get a description for.
 * @return a string representing the filetype, this must <b>NOT</b>
 *         be free():ed by the caller!
 */
char const *LIBMTP_Get_Property_Description(LIBMTP_property_t inproperty)
{
    propertymap_t *_current;

    _current = propertymap;

    while (_current != NULL)
    {
        if (_current->id == inproperty)
        {
            return _current->description;
        }
        _current = _current->next;
    }

    return "Unknown property";
}

/**
 * This function will do its best to fit a 16bit
 * value into a PTP object property if the property
 * is limited in range or step sizes.
 */
/* 
static uint16_t adjust_u16(uint16_t val, PTPObjectPropDesc *opd)
{
switch (opd->FormFlag) {
case PTP_DPFF_Range:
if (val < opd->FORM.Range.MinimumValue.u16) {
return opd->FORM.Range.MinimumValue.u16;
}
if (val > opd->FORM.Range.MaximumValue.u16) {
return opd->FORM.Range.MaximumValue.u16;
}
// Round down to last step.
if (val % opd->FORM.Range.StepSize.u16 != 0) {
return val - (val % opd->FORM.Range.StepSize.u16);
}
return val;
break;
case PTP_DPFF_Enumeration:
{
int i;
uint16_t bestfit = opd->FORM.Enum.SupportedValue[0].u16;

for (i=0; i<opd->FORM.Enum.NumberOfValues; i++) {
if (val == opd->FORM.Enum.SupportedValue[i].u16) {
return val;
}
// Rough guess of best fit
if (opd->FORM.Enum.SupportedValue[i].u16 < val) {
bestfit = opd->FORM.Enum.SupportedValue[i].u16;
}
}
// Just some default that'll work.
return bestfit;
}
default:
// Will accept any value
break;
}
return val;
}
 */
/**
 * This function will do its best to fit a 32bit
 * value into a PTP object property if the property
 * is limited in range or step sizes.
 */
/* 
static uint32_t adjust_u32(uint32_t val, PTPObjectPropDesc *opd)
{
switch (opd->FormFlag) {
case PTP_DPFF_Range:
if (val < opd->FORM.Range.MinimumValue.u32) {
return opd->FORM.Range.MinimumValue.u32;
}
if (val > opd->FORM.Range.MaximumValue.u32) {
return opd->FORM.Range.MaximumValue.u32;
}
// Round down to last step.
if (val % opd->FORM.Range.StepSize.u32 != 0) {
return val - (val % opd->FORM.Range.StepSize.u32);
}
return val;
break;
case PTP_DPFF_Enumeration:
{
int i;
uint32_t bestfit = opd->FORM.Enum.SupportedValue[0].u32;

for (i=0; i<opd->FORM.Enum.NumberOfValues; i++) {
if (val == opd->FORM.Enum.SupportedValue[i].u32) {
return val;
}
// Rough guess of best fit
if (opd->FORM.Enum.SupportedValue[i].u32 < val) {
bestfit = opd->FORM.Enum.SupportedValue[i].u32;
}
}
// Just some default that'll work.
return bestfit;
}
default:
// Will accept any value
break;
}
return val;
}
 */
/**
 * This function returns a newly created ISO 8601 timestamp with the
 * current time in as high precision as possible. It even adds
 * the time zone if it can.
 */
/* 
static char *get_iso8601_stamp(void)
{
time_t curtime;
struct tm *loctime;
char tmp[64];

curtime = time(NULL);
loctime = localtime(&curtime);
strftime (tmp, sizeof(tmp), "%Y%m%dT%H%M%S.0%z", loctime);
return strdup(tmp);
}
 */

/**
 * Gets the allowed values (range or enum) for a property
 * @param device a pointer to an MTP device
 * @param property the property to query
 * @param filetype the filetype of the object you want to set values for
 * @param allowed_vals pointer to a LIBMTP_allowed_values_t struct to
 *        receive the allowed values.  Call LIBMTP_destroy_allowed_values_t
 *        on this on successful completion.
 * @return 0 on success, any other value means failure
 */
int LIBMTP_Get_Allowed_Property_Values(LIBMTP_mtpdevice_t *device,
                                       LIBMTP_property_t const property,
                                       LIBMTP_filetype_t const filetype,
                                       LIBMTP_allowed_values_t *allowed_vals)
{
    PTPObjectPropDesc opd;
    uint16_t ret = 0;

    ret = ptp_mtp_getobjectpropdesc(device->params,
                                    map_libmtp_property_to_ptp_property
                                    (property), map_libmtp_type_to_ptp_type
                                    (filetype), &opd);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "LIBMTP_Get_Allowed_Property_Values(): could not get property description.");
        return  - 1;
    }

    if (opd.FormFlag == PTP_OPFF_Enumeration)
    {
        int i = 0;

        allowed_vals->is_range = 0;
        allowed_vals->num_entries = opd.FORM.Enum.NumberOfValues;

        switch (opd.DataType)
        {
            case PTP_DTC_INT8:
                allowed_vals->i8vals = kmalloc(sizeof(int8_t)
                    *opd.FORM.Enum.NumberOfValues, GFP_KERNEL);
                allowed_vals->datatype = LIBMTP_DATATYPE_INT8;
                break;
            case PTP_DTC_UINT8:
                allowed_vals->u8vals = kmalloc(sizeof(uint8_t)
                    *opd.FORM.Enum.NumberOfValues, GFP_KERNEL);
                allowed_vals->datatype = LIBMTP_DATATYPE_UINT8;
                break;
            case PTP_DTC_INT16:
                allowed_vals->i16vals = kmalloc(sizeof(int16_t)
                    *opd.FORM.Enum.NumberOfValues, GFP_KERNEL);
                allowed_vals->datatype = LIBMTP_DATATYPE_INT16;
                break;
            case PTP_DTC_UINT16:
                allowed_vals->u16vals = kmalloc(sizeof(uint16_t)
                    *opd.FORM.Enum.NumberOfValues, GFP_KERNEL);
                allowed_vals->datatype = LIBMTP_DATATYPE_UINT16;
                break;
            case PTP_DTC_INT32:
                allowed_vals->i32vals = kmalloc(sizeof(int32_t)
                    *opd.FORM.Enum.NumberOfValues, GFP_KERNEL);
                allowed_vals->datatype = LIBMTP_DATATYPE_INT32;
                break;
            case PTP_DTC_UINT32:
                allowed_vals->u32vals = kmalloc(sizeof(uint32_t)
                    *opd.FORM.Enum.NumberOfValues, GFP_KERNEL);
                allowed_vals->datatype = LIBMTP_DATATYPE_UINT32;
                break;
            case PTP_DTC_INT64:
                allowed_vals->i64vals = kmalloc(sizeof(int64_t)
                    *opd.FORM.Enum.NumberOfValues, GFP_KERNEL);
                allowed_vals->datatype = LIBMTP_DATATYPE_INT64;
                break;
            case PTP_DTC_UINT64:
                allowed_vals->u64vals = kmalloc(sizeof(uint64_t)
                    *opd.FORM.Enum.NumberOfValues, GFP_KERNEL);
                allowed_vals->datatype = LIBMTP_DATATYPE_UINT64;
                break;
        }

        for (i = 0; i < opd.FORM.Enum.NumberOfValues; i++)
        {
            switch (opd.DataType)
            {
                case PTP_DTC_INT8:
                    allowed_vals->i8vals[i] =
                        opd.FORM.Enum.SupportedValue[i].i8;
                    break;
                case PTP_DTC_UINT8:
                    allowed_vals->u8vals[i] =
                        opd.FORM.Enum.SupportedValue[i].u8;
                    break;
                case PTP_DTC_INT16:
                    allowed_vals->i16vals[i] =
                        opd.FORM.Enum.SupportedValue[i].i16;
                    break;
                case PTP_DTC_UINT16:
                    allowed_vals->u16vals[i] =
                        opd.FORM.Enum.SupportedValue[i].u16;
                    break;
                case PTP_DTC_INT32:
                    allowed_vals->i32vals[i] =
                        opd.FORM.Enum.SupportedValue[i].i32;
                    break;
                case PTP_DTC_UINT32:
                    allowed_vals->u32vals[i] =
                        opd.FORM.Enum.SupportedValue[i].u32;
                    break;
                case PTP_DTC_INT64:
                    allowed_vals->i64vals[i] =
                        opd.FORM.Enum.SupportedValue[i].i64;
                    break;
                case PTP_DTC_UINT64:
                    allowed_vals->u64vals[i] =
                        opd.FORM.Enum.SupportedValue[i].u64;
                    break;
            }
        }
        ptp_free_objectpropdesc(&opd);
        return 0;
    }
    else if (opd.FormFlag == PTP_OPFF_Range)
    {
        allowed_vals->is_range = 1;

        switch (opd.DataType)
        {
            case PTP_DTC_INT8:
                allowed_vals->i8min = opd.FORM.Range.MinimumValue.i8;
                allowed_vals->i8max = opd.FORM.Range.MaximumValue.i8;
                allowed_vals->i8step = opd.FORM.Range.StepSize.i8;
                allowed_vals->datatype = LIBMTP_DATATYPE_INT8;
                break;
            case PTP_DTC_UINT8:
                allowed_vals->u8min = opd.FORM.Range.MinimumValue.u8;
                allowed_vals->u8max = opd.FORM.Range.MaximumValue.u8;
                allowed_vals->u8step = opd.FORM.Range.StepSize.u8;
                allowed_vals->datatype = LIBMTP_DATATYPE_UINT8;
                break;
            case PTP_DTC_INT16:
                allowed_vals->i16min = opd.FORM.Range.MinimumValue.i16;
                allowed_vals->i16max = opd.FORM.Range.MaximumValue.i16;
                allowed_vals->i16step = opd.FORM.Range.StepSize.i16;
                allowed_vals->datatype = LIBMTP_DATATYPE_INT16;
                break;
            case PTP_DTC_UINT16:
                allowed_vals->u16min = opd.FORM.Range.MinimumValue.u16;
                allowed_vals->u16max = opd.FORM.Range.MaximumValue.u16;
                allowed_vals->u16step = opd.FORM.Range.StepSize.u16;
                allowed_vals->datatype = LIBMTP_DATATYPE_UINT16;
                break;
            case PTP_DTC_INT32:
                allowed_vals->i32min = opd.FORM.Range.MinimumValue.i32;
                allowed_vals->i32max = opd.FORM.Range.MaximumValue.i32;
                allowed_vals->i32step = opd.FORM.Range.StepSize.i32;
                allowed_vals->datatype = LIBMTP_DATATYPE_INT32;
                break;
            case PTP_DTC_UINT32:
                allowed_vals->u32min = opd.FORM.Range.MinimumValue.u32;
                allowed_vals->u32max = opd.FORM.Range.MaximumValue.u32;
                allowed_vals->u32step = opd.FORM.Range.StepSize.u32;
                allowed_vals->datatype = LIBMTP_DATATYPE_UINT32;
                break;
            case PTP_DTC_INT64:
                allowed_vals->i64min = opd.FORM.Range.MinimumValue.i64;
                allowed_vals->i64max = opd.FORM.Range.MaximumValue.i64;
                allowed_vals->i64step = opd.FORM.Range.StepSize.i64;
                allowed_vals->datatype = LIBMTP_DATATYPE_INT64;
                break;
            case PTP_DTC_UINT64:
                allowed_vals->u64min = opd.FORM.Range.MinimumValue.u64;
                allowed_vals->u64max = opd.FORM.Range.MaximumValue.u64;
                allowed_vals->u64step = opd.FORM.Range.StepSize.u64;
                allowed_vals->datatype = LIBMTP_DATATYPE_UINT64;
                break;
        }
        return 0;
    }
    else
    {
        return  - 1;
    }
}

/**
 * Destroys a LIBMTP_allowed_values_t struct
 * @param allowed_vals the struct to destroy
 */
void LIBMTP_destroy_allowed_values_t(LIBMTP_allowed_values_t *allowed_vals)
{
    if (!allowed_vals->is_range)
    {
        switch (allowed_vals->datatype)
        {
            case LIBMTP_DATATYPE_INT8:
                if (allowed_vals->i8vals)
                {
                    kfree(allowed_vals->i8vals);
                }
                break;
            case LIBMTP_DATATYPE_UINT8:
                if (allowed_vals->u8vals)
                {
                    kfree(allowed_vals->u8vals);
                }
                break;
            case LIBMTP_DATATYPE_INT16:
                if (allowed_vals->i16vals)
                {
                    kfree(allowed_vals->i16vals);
                }
                break;
            case LIBMTP_DATATYPE_UINT16:
                if (allowed_vals->u16vals)
                {
                    kfree(allowed_vals->u16vals);
                }
                break;
            case LIBMTP_DATATYPE_INT32:
                if (allowed_vals->i32vals)
                {
                    kfree(allowed_vals->i32vals);
                }
                break;
            case LIBMTP_DATATYPE_UINT32:
                if (allowed_vals->u32vals)
                {
                    kfree(allowed_vals->u32vals);
                }
                break;
            case LIBMTP_DATATYPE_INT64:
                if (allowed_vals->i64vals)
                {
                    kfree(allowed_vals->i64vals);
                }
                break;
            case LIBMTP_DATATYPE_UINT64:
                if (allowed_vals->u64vals)
                {
                    kfree(allowed_vals->u64vals);
                }
                break;
        }
    }
}

/**
 * Determine if a property is supported for a given file type
 * @param device a pointer to an MTP device
 * @param property the property to query
 * @param filetype the filetype of the object you want to set values for
 * @return 0 if not supported, positive if supported, negative on error
 */
int LIBMTP_Is_Property_Supported(LIBMTP_mtpdevice_t *device, LIBMTP_property_t
                                 const property, LIBMTP_filetype_t const
                                 filetype)
{
    uint16_t *props = NULL;
    uint32_t propcnt = 0;
    uint16_t ret = 0;
    int i = 0;
    int supported = 0;
    uint16_t ptp_prop = map_libmtp_property_to_ptp_property(property);

    ret = ptp_mtp_getobjectpropssupported(device->params,
        map_libmtp_type_to_ptp_type(filetype), &propcnt, &props);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "LIBMTP_Is_Property_Supported(): could not get properties supported.");
        return  - 1;
    }

    for (i = 0; i < propcnt; i++)
    {
        if (props[i] == ptp_prop)
        {
            supported = 1;
            break;
        }
    }

    kfree(props);

    return supported;
}

/**
 * Retrieves a string from an object
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @return valid string or NULL on failure. The returned string
 *         must bee <code>free()</code>:ed by the caller after
 *         use.
 */
char *LIBMTP_Get_String_From_Object(LIBMTP_mtpdevice_t *device, uint32_t const
                                    object_id, LIBMTP_property_t const
                                    attribute_id)
{
    return get_string_from_object(device, object_id, attribute_id);
}

/**
 * Retrieves an unsigned 64-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value_default Default value to return on failure
 * @return the value
 */
uint64_t LIBMTP_Get_u64_From_Object(LIBMTP_mtpdevice_t *device, uint32_t const
                                    object_id, LIBMTP_property_t const
                                    attribute_id, uint64_t const value_default)
{
    return get_u64_from_object(device, object_id,
                               map_libmtp_property_to_ptp_property(attribute_id)
                               , value_default);
}

/**
 * Retrieves an unsigned 32-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value_default Default value to return on failure
 * @return the value
 */
uint32_t LIBMTP_Get_u32_From_Object(LIBMTP_mtpdevice_t *device, uint32_t const
                                    object_id, LIBMTP_property_t const
                                    attribute_id, uint32_t const value_default)
{
    return get_u32_from_object(device, object_id,
                               map_libmtp_property_to_ptp_property(attribute_id)
                               , value_default);
}

/**
 * Retrieves an unsigned 16-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value_default Default value to return on failure
 * @return a value
 */
uint16_t LIBMTP_Get_u16_From_Object(LIBMTP_mtpdevice_t *device, uint32_t const
                                    object_id, LIBMTP_property_t const
                                    attribute_id, uint16_t const value_default)
{
    return get_u16_from_object(device, object_id,
                               map_libmtp_property_to_ptp_property(attribute_id)
                               , value_default);
}

/**
 * Retrieves an unsigned 8-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value_default Default value to return on failure
 * @return a value
 */
uint8_t LIBMTP_Get_u8_From_Object(LIBMTP_mtpdevice_t *device, uint32_t const
                                  object_id, LIBMTP_property_t const
                                  attribute_id, uint8_t const value_default)
{
    return get_u8_from_object(device, object_id,
                              map_libmtp_property_to_ptp_property(attribute_id),
                              value_default);
}

/**
 * Sets an object attribute from a string
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param string string value to set
 * @return 0 on success, any other value means failure
 */
int LIBMTP_Set_Object_String(LIBMTP_mtpdevice_t *device, uint32_t const
                             object_id, LIBMTP_property_t const attribute_id,
                             char const *const string)
{
    return set_object_string(device, object_id,
                             map_libmtp_property_to_ptp_property(attribute_id),
                             string);
}


/**
 * Sets an object attribute from an unsigned 32-bit integer
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value 32-bit unsigned integer to set
 * @return 0 on success, any other value means failure
 */
int LIBMTP_Set_Object_u32(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
                          LIBMTP_property_t const attribute_id, uint32_t const
                          value)
{
    return set_object_u32(device, object_id,
                          map_libmtp_property_to_ptp_property(attribute_id),
                          value);
}

/**
 * Sets an object attribute from an unsigned 16-bit integer
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value 16-bit unsigned integer to set
 * @return 0 on success, any other value means failure
 */
int LIBMTP_Set_Object_u16(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
                          LIBMTP_property_t const attribute_id, uint16_t const
                          value)
{
    return set_object_u16(device, object_id,
                          map_libmtp_property_to_ptp_property(attribute_id),
                          value);
}

/**
 * Sets an object attribute from an unsigned 8-bit integer
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id MTP attribute ID
 * @param value 8-bit unsigned integer to set
 * @return 0 on success, any other value means failure
 */
int LIBMTP_Set_Object_u8(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
                         LIBMTP_property_t const attribute_id, uint8_t const
                         value)
{
    return set_object_u8(device, object_id, map_libmtp_property_to_ptp_property
                         (attribute_id), value);
}

/**
 * Retrieves a string from an object
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @return valid string or NULL on failure. The returned string
 *         must bee <code>free()</code>:ed by the caller after
 *         use.
 */
static char *get_string_from_object(LIBMTP_mtpdevice_t *device, uint32_t const
                                    object_id, uint16_t const attribute_id)
{
    PTPPropertyValue propval;
    char *retstring = NULL;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;
    MTPProperties *prop;

    if (device == NULL || object_id == 0)
    {
        return NULL;
    }

    prop = ptp_find_object_prop_in_cache(params, object_id, attribute_id);
    if (prop)
    {
        if (prop->propval.str != NULL)
        {
            return strdup(prop->propval.str);
        }
        else
        {
            return NULL;
        }
    }

    ret = ptp_mtp_getobjectpropvalue(params, object_id, attribute_id, &propval,
                                     PTP_DTC_STR);
    if (ret == PTP_RC_OK)
    {
        if (propval.str != NULL)
        {
            retstring = (char*)strdup(propval.str);
            kfree(propval.str);
        }
    }
    else
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "get_string_from_object(): could not get object string.");
    }

    return retstring;
}

/**
 * Retrieves an unsigned 64-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value_default Default value to return on failure
 * @return the value
 */
static uint64_t get_u64_from_object(LIBMTP_mtpdevice_t *device, uint32_t const
                                    object_id, uint16_t const attribute_id,
                                    uint64_t const value_default)
{
    PTPPropertyValue propval;
    uint64_t retval = value_default;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;
    MTPProperties *prop;

    if (device == NULL)
    {
        return value_default;
    }

    prop = ptp_find_object_prop_in_cache(params, object_id, attribute_id);
    if (prop)
    {
        return prop->propval.u64;
    }

    ret = ptp_mtp_getobjectpropvalue(params, object_id, attribute_id, &propval,
                                     PTP_DTC_UINT64);
    if (ret == PTP_RC_OK)
    {
        retval = propval.u64;
    }
    else
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "get_u64_from_object(): could not get unsigned 64bit integer from object.");
    }

    return retval;
}

/**
 * Retrieves an unsigned 32-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value_default Default value to return on failure
 * @return the value
 */
static uint32_t get_u32_from_object(LIBMTP_mtpdevice_t *device, uint32_t const
                                    object_id, uint16_t const attribute_id,
                                    uint32_t const value_default)
{
    PTPPropertyValue propval;
    uint32_t retval = value_default;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;
    MTPProperties *prop;

    if (device == NULL)
    {
        return value_default;
    }

    prop = ptp_find_object_prop_in_cache(params, object_id, attribute_id);
    if (prop)
    {
        return prop->propval.u32;
    }

    ret = ptp_mtp_getobjectpropvalue(params, object_id, attribute_id, &propval,
                                     PTP_DTC_UINT32);
    if (ret == PTP_RC_OK)
    {
        retval = propval.u32;
    }
    else
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "get_u32_from_object(): could not get unsigned 32bit integer from object.");
    }
    return retval;
}

/**
 * Retrieves an unsigned 16-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value_default Default value to return on failure
 * @return a value
 */
static uint16_t get_u16_from_object(LIBMTP_mtpdevice_t *device, uint32_t const
                                    object_id, uint16_t const attribute_id,
                                    uint16_t const value_default)
{
    PTPPropertyValue propval;
    uint16_t retval = value_default;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;
    MTPProperties *prop;

    if (device == NULL)
    {
        return value_default;
    }

    // This O(n) search should not be used so often, since code
    // using the cached properties don't usually call this function.
    prop = ptp_find_object_prop_in_cache(params, object_id, attribute_id);
    if (prop)
    {
        return prop->propval.u16;
    }

    ret = ptp_mtp_getobjectpropvalue(params, object_id, attribute_id, &propval,
                                     PTP_DTC_UINT16);
    if (ret == PTP_RC_OK)
    {
        retval = propval.u16;
    }
    else
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "get_u16_from_object(): could not get unsigned 16bit integer from object.");
    }

    return retval;
}

/**
 * Retrieves an unsigned 8-bit integer from an object attribute
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value_default Default value to return on failure
 * @return a value
 */
static uint8_t get_u8_from_object(LIBMTP_mtpdevice_t *device, uint32_t const
                                  object_id, uint16_t const attribute_id,
                                  uint8_t const value_default)
{
    PTPPropertyValue propval;
    uint8_t retval = value_default;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;
    MTPProperties *prop;

    if (device == NULL)
    {
        return value_default;
    }

    // This O(n) search should not be used so often, since code
    // using the cached properties don't usually call this function.
    prop = ptp_find_object_prop_in_cache(params, object_id, attribute_id);
    if (prop)
    {
        return prop->propval.u8;
    }

    ret = ptp_mtp_getobjectpropvalue(params, object_id, attribute_id, &propval,
                                     PTP_DTC_UINT8);
    if (ret == PTP_RC_OK)
    {
        retval = propval.u8;
    }
    else
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "get_u8_from_object(): could not get unsigned 8bit integer from object.");
    }

    return retval;
}

/**
 * Sets an object attribute from a string
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param string string value to set
 * @return 0 on success, any other value means failure
 */
static int set_object_string(LIBMTP_mtpdevice_t *device, uint32_t const
                             object_id, uint16_t const attribute_id, char
                             const*const string)
{
    PTPPropertyValue propval;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;

    if (device == NULL || string == NULL)
    {
        return  - 1;
    }

    if (!ptp_operation_issupported(params, PTP_OC_MTP_SetObjectPropValue))
    {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                "set_object_string(): could not set object string: ""PTP_OC_MTP_SetObjectPropValue not supported.");
        return  - 1;
    }
    propval.str = (char*)string;
    ret = ptp_mtp_setobjectpropvalue(params, object_id, attribute_id, &propval,
                                     PTP_DTC_STR);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "set_object_string(): could not set object string.");
        return  - 1;
    }

    return 0;
}


/**
 * Sets an object attribute from an unsigned 32-bit integer
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value 32-bit unsigned integer to set
 * @return 0 on success, any other value means failure
 */
static int set_object_u32(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
                          uint16_t const attribute_id, uint32_t const value)
{
    PTPPropertyValue propval;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;

    if (device == NULL)
    {
        return  - 1;
    }

    if (!ptp_operation_issupported(params, PTP_OC_MTP_SetObjectPropValue))
    {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                "set_object_u32(): could not set unsigned 32bit integer property: ""PTP_OC_MTP_SetObjectPropValue not supported.");
        return  - 1;
    }

    propval.u32 = value;
    ret = ptp_mtp_setobjectpropvalue(params, object_id, attribute_id, &propval,
                                     PTP_DTC_UINT32);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "set_object_u32(): could not set unsigned 32bit integer property.");
        return  - 1;
    }

    return 0;
}

/**
 * Sets an object attribute from an unsigned 16-bit integer
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value 16-bit unsigned integer to set
 * @return 0 on success, any other value means failure
 */
static int set_object_u16(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
                          uint16_t const attribute_id, uint16_t const value)
{
    PTPPropertyValue propval;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;

    if (device == NULL)
    {
        return 1;
    }

    if (!ptp_operation_issupported(params, PTP_OC_MTP_SetObjectPropValue))
    {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                "set_object_u16(): could not set unsigned 16bit integer property: ""PTP_OC_MTP_SetObjectPropValue not supported.");
        return  - 1;
    }
    propval.u16 = value;
    ret = ptp_mtp_setobjectpropvalue(params, object_id, attribute_id, &propval,
                                     PTP_DTC_UINT16);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "set_object_u16(): could not set unsigned 16bit integer property.");
        return 1;
    }

    return 0;
}

/**
 * Sets an object attribute from an unsigned 8-bit integer
 *
 * @param device a pointer to an MTP device.
 * @param object_id Object reference
 * @param attribute_id PTP attribute ID
 * @param value 8-bit unsigned integer to set
 * @return 0 on success, any other value means failure
 */
static int set_object_u8(LIBMTP_mtpdevice_t *device, uint32_t const object_id,
                         uint16_t const attribute_id, uint8_t const value)
{
    PTPPropertyValue propval;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;

    if (device == NULL)
    {
        return 1;
    }

    if (!ptp_operation_issupported(params, PTP_OC_MTP_SetObjectPropValue))
    {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                "set_object_u8(): could not set unsigned 8bit integer property: ""PTP_OC_MTP_SetObjectPropValue not supported.");
        return  - 1;
    }
    propval.u8 = value;
    ret = ptp_mtp_setobjectpropvalue(params, object_id, attribute_id, &propval,
                                     PTP_DTC_UINT8);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "set_object_u8(): could not set unsigned 8bit integer property.");
        return 1;
    }

    return 0;
}

/**************************************************************************
 *                                                                        *
 **************************************************************************/

/**
 * Parses the extension descriptor, there may be stuff in
 * this that we want to know about.
 */
static void parse_extension_descriptor(LIBMTP_mtpdevice_t *mtpdevice, char*desc)
{
    int start = 0;
    int end = 0;

    if (desc == NULL)
        return;

    /* descriptors are divided by semicolons */
    while (end < strlen(desc))
    {
        while (desc[end] != ';' && end < strlen(desc))
        {
            end++;
        }
        if (end < strlen(desc))
        {
            char *element = strndup(desc + start, end - start);
                        
            if (element)
            {
                int i = 0;
                printk("  Element: \"%s\"\n", element);

                /* Parse for an extension */
                while (element[i] != ':' && i < strlen(element))
                {
                    i++;
                }
                if (i < strlen(element))
                {
                    char *name = strndup(element, i);
                    int majstart = i + 1;
                    printk("    Extension: \"%s\"\n", name);

                    /* Parse for minor/major punctuation mark for this extension */
                    while (element[i] != '.' && i < strlen(element))
                    {
                        i++;
                    }
                    if (i > majstart && i < strlen(element))
                    {
                        LIBMTP_device_extension_t *extension;
                        int major = 0;
                        int minor = 0;
                        char *majorstr = strndup(element + majstart, i -
                            majstart);
                        char *minorstr = strndup(element + i + 1, strlen
                            (element) - i - 1);

                        /*
                        major = atoi(majorstr);
                        minor = atoi(minorstr);
                         */
                        major = simple_strtol(majorstr, NULL, 10);
                        minor = simple_strtol(minorstr, NULL, 10);

                        extension = kmalloc(sizeof(LIBMTP_device_extension_t),
                            GFP_KERNEL);
                        extension->name = name;
                        extension->major = major;
                        extension->minor = minor;
                        extension->next = NULL;
                        if (mtpdevice->extensions == NULL)
                        {
                            mtpdevice->extensions = extension;
                        }
                        else
                        {
                            LIBMTP_device_extension_t *tmp = mtpdevice
                                ->extensions;
                            while (tmp->next != NULL)
                            {
                                tmp = tmp->next;
                            }
                            tmp->next = extension;
                        }
                        printk(
                               "    Major: \"%s\" (parsed %d) Minor: \"%s\" (parsed %d)\n", majorstr, major, minorstr, minor);
                    }
                    else
                    {
                        LIBMTP_ERROR(
                                     "LIBMTP ERROR: couldnt parse extension %s\n", element);
                    }
                }
                kfree(element);
            }
        }
        end++;
        start = end;
    }
}

/**************************************************************************
 *                                                                        *
 **************************************************************************/
/**
 * This closes and releases an allocated MTP device.
 * @param device a pointer to the MTP device to release.
 */
void LIBMTP_Release_Device_List(LIBMTP_mtpdevice_t *device)
{
    if (device != NULL)
    {
        if (device->next != NULL)
        {
            LIBMTP_Release_Device_List(device->next);
        }

        LIBMTP_Release_Device(device);
    }
}


/**
 * This closes and releases an allocated MTP device.
 * @param device a pointer to the MTP device to release.
 */
void LIBMTP_Release_Device(LIBMTP_mtpdevice_t *device)
{
    PTPParams *params = (PTPParams*)device->params;
    PTP_USB *ptp_usb = (PTP_USB*)device->usbinfo;

    // close_device(ptp_usb, params);
    // Clear error stack
    LIBMTP_Clear_Errorstack(device);
    // Free iconv() converters...
    #ifdef HAVE_ICONV  
        iconv_close(params->cd_locale_to_ucs2);
        iconv_close(params->cd_ucs2_to_locale);
    #endif 
    kfree(ptp_usb);
    ptp_free_params(params);
    free_storage_list(device);
    // Free extension list...
    if (device->extensions != NULL)
    {
        LIBMTP_device_extension_t *tmp = device->extensions;

        while (tmp != NULL)
        {
            LIBMTP_device_extension_t *next = tmp->next;

            if (tmp->name)
            {
                kfree(tmp->name);
            }
            kfree(tmp);
            tmp = next;
        }
    }
    kfree(device);
}

/**
 * This can be used by any libmtp-intrinsic code that
 * need to stack up an error on the stack. You are only
 * supposed to add errors to the error stack using this
 * function, do not create and reference error entries
 * directly.
 */
static void add_error_to_errorstack(LIBMTP_mtpdevice_t *device,
                                    LIBMTP_error_number_t errornumber, char
                                    const *const error_text)
{
    LIBMTP_error_t *newerror;

    if (device == NULL)
    {
        LIBMTP_ERROR("LIBMTP PANIC: Trying to add error to a NULL device!\n");
        return ;
    }
    newerror = (LIBMTP_error_t*)kmalloc(sizeof(LIBMTP_error_t), GFP_KERNEL);
    newerror->errornumber = errornumber;
    newerror->error_text = strdup((char*)error_text);
    newerror->next = NULL;
    if (device->errorstack == NULL)
    {
        device->errorstack = newerror;
    }
    else
    {
        LIBMTP_error_t *tmp = device->errorstack;

        while (tmp->next != NULL)
        {
            tmp = tmp->next;
        }
        tmp->next = newerror;
    }
}

/**
 * Adds an error from the PTP layer to the error stack.
 */
static void add_ptp_error_to_errorstack(LIBMTP_mtpdevice_t *device, uint16_t
                                        ptp_error, char const *const error_text)
{
    if (device == NULL)
    {
        LIBMTP_ERROR(
                     "LIBMTP PANIC: Trying to add PTP error to a NULL device!\n");
        return ;
    }
    else
    {
        char outstr[256];
        snprintf(outstr, sizeof(outstr), "PTP Layer error %04x: %s", ptp_error,
                 error_text);
        outstr[sizeof(outstr) - 1] = '\0';
        add_error_to_errorstack(device, LIBMTP_ERROR_PTP_LAYER, outstr);
        add_error_to_errorstack(device, LIBMTP_ERROR_PTP_LAYER, 
                                "(Look this up in ptp.h for an explanation.)");
    }
}

/**
 * This returns the error stack for a device in case you
 * need to either reference the error numbers (e.g. when
 * creating multilingual apps with multiple-language text
 * representations for each error number) or when you need
 * to build a multi-line error text widget or something like
 * that. You need to call the <code>LIBMTP_Clear_Errorstack</code>
 * to clear it when you're finished with it.
 * @param device a pointer to the MTP device to get the error
 *        stack for.
 * @return the error stack or NULL if there are no errors
 *         on the stack.
 * @see LIBMTP_Clear_Errorstack()
 * @see LIBMTP_Dump_Errorstack()
 */
LIBMTP_error_t *LIBMTP_Get_Errorstack(LIBMTP_mtpdevice_t *device)
{
    if (device == NULL)
    {
        LIBMTP_ERROR(
                     "LIBMTP PANIC: Trying to get the error stack of a NULL device!\n");
        return NULL;
    }
    return device->errorstack;
}

/**
 * This function clears the error stack of a device and frees
 * any memory used by it. Call this when you're finished with
 * using the errors.
 * @param device a pointer to the MTP device to clear the error
 *        stack for.
 */
void LIBMTP_Clear_Errorstack(LIBMTP_mtpdevice_t *device)
{
    if (device == NULL)
    {
        LIBMTP_ERROR(
                     "LIBMTP PANIC: Trying to clear the error stack of a NULL device!\n");
    }
    else
    {
        LIBMTP_error_t *tmp = device->errorstack;

        while (tmp != NULL)
        {
            LIBMTP_error_t *tmp2;

            if (tmp->error_text != NULL)
            {
                kfree(tmp->error_text);
            }
            tmp2 = tmp;
            tmp = tmp->next;
            kfree(tmp2);
        }
        device->errorstack = NULL;
    }
}

/**
 * This function dumps the error stack to <code>stderr</code>.
 * (You still have to clear the stack though.)
 * @param device a pointer to the MTP device to dump the error
 *        stack for.
 */
void LIBMTP_Dump_Errorstack(LIBMTP_mtpdevice_t *device)
{
    if (device == NULL)
    {
        LIBMTP_ERROR(
                     "LIBMTP PANIC: Trying to dump the error stack of a NULL device!\n");
    }
    else
    {
        LIBMTP_error_t *tmp = device->errorstack;

        while (tmp != NULL)
        {
            if (tmp->error_text != NULL)
            {
                LIBMTP_ERROR("Error %d: %s\n", tmp->errornumber, tmp
                             ->error_text);
            }
            else
            {
                LIBMTP_ERROR("Error %d: (unknown)\n", tmp->errornumber);
            }
            tmp = tmp->next;
        }
    }
}

#if 0
/**
 * This command gets all handles and stuff by FAST directory retrieveal
 * which is available by getting all metadata for object
 * <code>0xffffffff</code> which simply means "all metadata for all objects".
 * This works on the vast majority of MTP devices (there ARE exceptions!)
 * and is quite quick. Check the error stack to see if there were
 * problems getting the metadata.
 * @return 0 if all was OK, -1 on failure.
 */
static int get_all_metadata_fast(LIBMTP_mtpdevice_t *device, uint32_t storage)
{
    PTPParams *params = (PTPParams*)device->params;
    int cnt = 0;
    int i, j, nrofprops;
    uint32_t lasthandle = 0xffffffff;
    MTPProperties *props = NULL;
    MTPProperties *prop;
    uint16_t ret;
    int oldtimeout;
    PTP_USB *ptp_usb = (PTP_USB*)device->usbinfo;

    /* The follow request causes the device to generate
     * a list of very file on the device and return it
     * in a single response.
     *
     * Some slow devices as well as devices with very
     * large file systems can easily take longer then
     * the standard timeout value before it is able
     * to return a response.
     *
     * Temporarly set timeout to allow working with
     * widest range of devices.
     */
    get_usb_device_timeout(ptp_usb, &oldtimeout);
    set_usb_device_timeout(ptp_usb, 60000);

    ret = ptp_mtp_getobjectproplist(params, 0xffffffff, &props, &nrofprops);
    set_usb_device_timeout(ptp_usb, oldtimeout);

    if (ret == PTP_RC_MTP_Specification_By_Group_Unsupported)
    {
        // What's the point in the device implementing this command if
        // you cannot use it to get all props for AT LEAST one object?
        // Well, whatever...
        add_ptp_error_to_errorstack(device, ret, "get_all_metadata_fast(): "
                                    "cannot retrieve all metadata for an object on this device.");
        return  - 1;
    }

    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, "get_all_metadata_fast(): "
                                    "could not get proplist of all objects.");
        return  - 1;
    }

    if (props == NULL && nrofprops != 0)
    {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                "get_all_metadata_fast(): "
                                "call to ptp_mtp_getobjectproplist() returned "
                                "inconsistent results.");
        return  - 1;
    }

    /*
     * We count the number of objects by counting the ObjectHandle
     * references, whenever it changes we get a new object, when it's
     * the same, it is just different properties of the same object.
     */

    prop = props;
    for (i = 0; i < nrofprops; i++)
    {
        if (lasthandle != prop->ObjectHandle)
        {
            cnt++;
            lasthandle = prop->ObjectHandle;
        }
        prop++;
    }

    lasthandle = 0xffffffff;
    // params->objects = calloc (sizeof(PTPObject),cnt);
    params->objects = kmalloc(sizeof(PTPObject) *cnt, GFP_KERNEL);
    memset(params->objects, 0, sizeof(PTPObject) *cnt);

    prop = props;
    i =  - 1;

    for (j = 0; j < nrofprops; j++)
    {
        if (lasthandle != prop->ObjectHandle)
        {
            if (i >= 0)
            {
                params->objects[i].flags |= PTPOBJECT_OBJECTINFO_LOADED;
                if (!params->objects[i].oi.Filename)
                {
                    /* I have one such file on my Creative (Marcus) */
                    params->objects[i].oi.Filename = strdup("<null>");
                }
            }
            i++;
            lasthandle = prop->ObjectHandle;
            params->objects[i].oid = prop->ObjectHandle;
        }
        switch (prop->property)
        {
            case PTP_OPC_ParentObject:
                params->objects[i].oi.ParentObject = prop->propval.u32;
                params->objects[i].flags |= PTPOBJECT_PARENTOBJECT_LOADED;
                break;
            case PTP_OPC_ObjectFormat:
                params->objects[i].oi.ObjectFormat = prop->propval.u16;
                break;
            case PTP_OPC_ObjectSize:
                // We loose precision here, up to 32 bits! However the commands that
                // retrieve metadata for files and tracks will make sure that the
                // PTP_OPC_ObjectSize is read in and duplicated again.
                if (device->object_bitsize == 64)
                {
                    params->objects[i].oi.ObjectCompressedSize = (uint32_t)prop
                        ->propval.u64;
                }
                else
                {
                    params->objects[i].oi.ObjectCompressedSize = prop
                        ->propval.u32;
                }
                break;
            case PTP_OPC_StorageID:
                params->objects[i].oi.StorageID = prop->propval.u32;
                params->objects[i].flags |= PTPOBJECT_STORAGEID_LOADED;
                break;
            case PTP_OPC_ObjectFileName:
                if (prop->propval.str != NULL)
                {
                    params->objects[i].oi.Filename = strdup(prop->propval.str);
                }
                break;
            default:
                {
                    MTPProperties *newprops;

                    /* Copy all of the other MTP oprierties into the per-object proplist */
                    if (params->objects[i].nrofmtpprops)
                    {
                        newprops = LIBMTP_realloc(params->objects[i].mtpprops, 
                            (params->objects[i].nrofmtpprops + 1) *sizeof
                            (MTPProperties), params
                            ->objects[i].nrofmtpprops*sizeof(MTPProperties));
                    }
                    else
                    {
                        // newprops = calloc(sizeof(MTPProperties),1);
                        newprops = kmalloc(sizeof(MTPProperties) *1, GFP_KERNEL)
                            ;
                        memset(newprops, 0, sizeof(MTPProperties));
                    }
                    if (!newprops)
                    {
                        return 0;
                    }
                    /* FIXME: error handling? */
                    params->objects[i].mtpprops = newprops;
                    memcpy(&params->objects[i].mtpprops[params
                           ->objects[i].nrofmtpprops], &props[j], sizeof
                           (props[j]));
                    params->objects[i].nrofmtpprops++;
                    params->objects[i].flags |= PTPOBJECT_MTPPROPLIST_LOADED;
                    break;
                }
        }
        prop++;
    }
    /* mark last entry also */
    params->objects[i].flags |= PTPOBJECT_OBJECTINFO_LOADED;
    params->nrofobjects = i + 1;
    /* The device might not give the list in linear ascending order */
    // ptp_objects_sort (params);
    return 0;
}

/**
 * This function will recurse through all the directories on the device,
 * starting at the root directory, gathering metadata as it moves along.
 * It works better on some devices that will only return data for a
 * certain directory and does not respect the option to get all metadata
 * for all objects.
 */
static void get_handles_recursively(LIBMTP_mtpdevice_t *device,
                                    PTPParams*params, uint32_t storageid,
                                    uint32_t parent)
{
    PTPObjectHandles currentHandles;
    int i = 0;
    uint16_t ret = ptp_getobjecthandles(params, storageid, PTP_GOH_ALL_FORMATS,
                                        parent, &currentHandles);

    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "get_handles_recursively(): could not get object handles.");
        return ;
    }

    if (currentHandles.Handler == NULL || currentHandles.n == 0)
    {
        return ;
    }

    // Now descend into any subdirectories found
    for (i = 0; i < currentHandles.n; i++)
    {
        PTPObject *ob;
        ret = ptp_object_want(params, currentHandles.Handler[i],
                              PTPOBJECT_OBJECTINFO_LOADED, &ob);
        if (ret == PTP_RC_OK)
        {
            if (ob->oi.ObjectFormat == PTP_OFC_Association)
            {
                get_handles_recursively(device, params, storageid,
                                        currentHandles.Handler[i]);
            }
        }
        else
        {
            add_error_to_errorstack(device, LIBMTP_ERROR_CONNECTING, 
                                    "Found a bad handle, trying to ignore it.");
        }
    }
    kfree(currentHandles.Handler);
}


/**
 * This function refresh the internal handle list whenever
 * the items stored inside the device is altered. On operations
 * that do not add or remove objects, this is typically not
 * called.
 * @param device a pointer to the MTP device to flush handles for.
 */
static void flush_handles(LIBMTP_mtpdevice_t *device)
{
    PTPParams *params = (PTPParams*)device->params;
    PTP_USB *ptp_usb = (PTP_USB*)device->usbinfo;
    int ret;
    uint32_t i;

    if (params->objects != NULL)
    {
        for (i = 0; i < params->nrofobjects; i++)
        {
            ptp_free_object(&params->objects[i]);
        }
        kfree(params->objects);
        params->objects = NULL;
        params->nrofobjects = 0;
    }

    if (ptp_operation_issupported(params, PTP_OC_MTP_GetObjPropList) &&
        !FLAG_BROKEN_MTPGETOBJPROPLIST(ptp_usb) &&
        !FLAG_BROKEN_MTPGETOBJPROPLIST_ALL(ptp_usb))
    {
        // Use the fast method. Ignore return value for now.
        ret = get_all_metadata_fast(device, PTP_GOH_ALL_STORAGE);
    }
    // If the previous failed or returned no objects, use classic
    // methods instead.
    if (params->nrofobjects == 0)
    {
        // Get all the handles using just standard commands.
        if (device->storage == NULL)
        {
            get_handles_recursively(device, params, PTP_GOH_ALL_STORAGE,
                                    PTP_GOH_ROOT_PARENT);
        }
        else
        {
            // Get handles for each storage in turn.
            LIBMTP_devicestorage_t *storage = device->storage;
            while (storage != NULL)
            {
                get_handles_recursively(device, params, storage->id,
                                        PTP_GOH_ROOT_PARENT);
                storage = storage->next;
            }
        }
    }

    /*
     * Loop over the handles, fix up any NULL filenames or
     * keywords, then attempt to locate some default folders
     * in the root directory of the primary storage.
     */
    for (i = 0; i < params->nrofobjects; i++)
    {
        PTPObject *ob,  *xob;

        ob = &params->objects[i];
        ret = ptp_object_want(params, params->objects[i].oid,
                              PTPOBJECT_OBJECTINFO_LOADED, &xob);
        if (ret != PTP_RC_OK)
        {
            LIBMTP_ERROR("broken! %x not found\n", params->objects[i].oid);
        }
        if (ob->oi.Filename == NULL)
        {
            ob->oi.Filename = strdup("<null>");
        }
        if (ob->oi.Keywords == NULL)
        {
            ob->oi.Keywords = strdup("<null>");
        }

        /* Ignore handles that point to non-folders */
        if (ob->oi.ObjectFormat != PTP_OFC_Association)
        {
            continue;
        }
        /* Only look in the root folder */
        if (ob->oi.ParentObject == 0xffffffffU)
        {
            LIBMTP_ERROR(
                         "object %x has parent 0xffffffff (-1) continuing anyway\n", ob->oid);
        }
        else if (ob->oi.ParentObject != 0x00000000U)
        {
            continue;
        }
        /* Only look in the primary storage */
        if (device->storage != NULL && ob->oi.StorageID != device->storage->id)
        {
            continue;
        }

        /* Is this the Music Folder */
        if (!strcasecmp(ob->oi.Filename, "My Music") || !strcasecmp(ob
            ->oi.Filename, "My_Music") || !strcasecmp(ob->oi.Filename, "Music"))
        {
            device->default_music_folder = ob->oid;
        }
        else if (!strcasecmp(ob->oi.Filename, "My Playlists") || !strcasecmp(ob
                 ->oi.Filename, "My_Playlists") || !strcasecmp(ob->oi.Filename,
                 "Playlists"))
        {
            device->default_playlist_folder = ob->oid;
        }
        else if (!strcasecmp(ob->oi.Filename, "My Pictures") || !strcasecmp(ob
                 ->oi.Filename, "My_Pictures") || !strcasecmp(ob->oi.Filename, 
                 "Pictures"))
        {
            device->default_picture_folder = ob->oid;
        }
        else if (!strcasecmp(ob->oi.Filename, "My Video") || !strcasecmp(ob
                 ->oi.Filename, "My_Video") || !strcasecmp(ob->oi.Filename, 
                 "Video"))
        {
            device->default_video_folder = ob->oid;
        }
        else if (!strcasecmp(ob->oi.Filename, "My Organizer") || !strcasecmp(ob
                 ->oi.Filename, "My_Organizer"))
        {
            device->default_organizer_folder = ob->oid;
        }
        else if (!strcasecmp(ob->oi.Filename, "ZENcast") || !strcasecmp(ob
                 ->oi.Filename, "Datacasts"))
        {
            device->default_zencast_folder = ob->oid;
        }
        else if (!strcasecmp(ob->oi.Filename, "My Albums") || !strcasecmp(ob
                 ->oi.Filename, "My_Albums") || !strcasecmp(ob->oi.Filename, 
                 "Albums"))
        {
            device->default_album_folder = ob->oid;
        }
        else if (!strcasecmp(ob->oi.Filename, "Text") || !strcasecmp(ob
                 ->oi.Filename, "Texts"))
        {
            device->default_text_folder = ob->oid;
        }
    }
}
#endif // #if 0

/**
 * This function traverses a devices storage list freeing up the
 * strings and the structs.
 * @param device a pointer to the MTP device to free the storage
 * list for.
 */
static void free_storage_list(LIBMTP_mtpdevice_t *device)
{
    LIBMTP_devicestorage_t *storage;
    LIBMTP_devicestorage_t *tmp;

    storage = device->storage;
    while (storage != NULL)
    {
        if (storage->StorageDescription != NULL)
        {
            kfree(storage->StorageDescription);
        }
        if (storage->VolumeIdentifier != NULL)
        {
            kfree(storage->VolumeIdentifier);
        }
        tmp = storage;
        storage = storage->next;
        kfree(tmp);
    }
    device->storage = NULL;
    device->num_storage = 0;

    return ;
}

/**
 * This function traverses a devices storage list freeing up the
 * strings and the structs.
 * @param device a pointer to the MTP device to free the storage
 * list for.
 */
static int sort_storage_by(LIBMTP_mtpdevice_t *device, int const sortby)
{
    LIBMTP_devicestorage_t *oldhead,  *ptr1,  *ptr2,  *newlist;

    if (device->storage == NULL)
    {
        return  - 1;
    }
    if (sortby == LIBMTP_STORAGE_SORTBY_NOTSORTED)
    {
        return 0;
    }

    oldhead = ptr1 = ptr2 = device->storage;

    newlist = NULL;

    while (oldhead != NULL)
    {
        ptr1 = ptr2 = oldhead;
        while (ptr1 != NULL)
        {

            if (sortby == LIBMTP_STORAGE_SORTBY_FREESPACE && ptr1
                ->FreeSpaceInBytes > ptr2->FreeSpaceInBytes)
            {
                ptr2 = ptr1;
            }
            if (sortby == LIBMTP_STORAGE_SORTBY_MAXSPACE && ptr1
                ->FreeSpaceInBytes > ptr2->FreeSpaceInBytes)
            {
                ptr2 = ptr1;
            }

            ptr1 = ptr1->next;
        }

        // Make our previous entries next point to our next
        if (ptr2->prev != NULL)
        {
            ptr1 = ptr2->prev;
            ptr1->next = ptr2->next;
        }
        else
        {
            oldhead = ptr2->next;
            if (oldhead != NULL)
            {
                oldhead->prev = NULL;
            }
        }

        // Make our next entries previous point to our previous
        ptr1 = ptr2->next;
        if (ptr1 != NULL)
        {
            ptr1->prev = ptr2->prev;
        }
        else
        {
            ptr1 = ptr2->prev;
            if (ptr1 != NULL)
            {
                ptr1->next = NULL;
            }
        }

        if (newlist == NULL)
        {
            newlist = ptr2;
            newlist->prev = NULL;
        }
        else
        {
            ptr2->prev = newlist;
            newlist->next = ptr2;
            newlist = newlist->next;
        }
    }

    if (newlist != NULL)
    {
        newlist->next = NULL;
        while (newlist->prev != NULL)
        {
            newlist = newlist->prev;
        }
        device->storage = newlist;
    }

    return 0;
}

/**
 * This function grabs the first writeable storageid from the
 * device storage list.
 * @param device a pointer to the MTP device to locate writeable
 *        storage for.
 * @param fitsize a file of this file must fit on the device.
 */
static uint32_t get_writeable_storageid(LIBMTP_mtpdevice_t *device, uint64_t
                                        fitsize)
{
    LIBMTP_devicestorage_t *storage;
    uint32_t store = 0x00000000; //Should this be 0xffffffffu instead?
    int subcall_ret;

    // See if there is some storage we can fit this file on.
    storage = device->storage;
    if (storage == NULL)
    {
        // Sometimes the storage just cannot be detected.
        store = 0x00000000U;
    }
    else
    {
        while (storage != NULL)
        {
            // These storages cannot be used.
            if (storage->StorageType == PTP_ST_FixedROM || storage->StorageType
                == PTP_ST_RemovableROM)
            {
                storage = storage->next;
                continue;
            }
            // Storage IDs with the lower 16 bits 0x0000 are not supposed
            // to be writeable.
            if ((storage->id &0x0000FFFFU) == 0x00000000U)
            {
                storage = storage->next;
                continue;
            }
            // Also check the access capability to avoid e.g. deletable only storages
            if (storage->AccessCapability == PTP_AC_ReadOnly || storage
                ->AccessCapability == PTP_AC_ReadOnly_with_Object_Deletion)
            {
                storage = storage->next;
                continue;
            }
            // Then see if we can fit the file.
            subcall_ret = check_if_file_fits(device, storage, fitsize);
            if (subcall_ret != 0)
            {
                storage = storage->next;
            }
            else
            {
                // We found a storage that is writable and can fit the file!
                break;
            }
        }
        if (storage == NULL)
        {
            add_error_to_errorstack(device, LIBMTP_ERROR_STORAGE_FULL, 
                                    "get_writeable_storageid(): "
                                    "all device storage is full or corrupt.");
            return  - 1;
        }
        store = storage->id;
    }

    return store;
}

/**
 * This function grabs the freespace from a certain storage in
 * device storage list.
 * @param device a pointer to the MTP device to free the storage
 * list for.
 * @param storageid the storage ID for the storage to flush and
 * get free space for.
 * @param freespace the free space on this storage will be returned
 * in this variable.
 */
static int get_storage_freespace(LIBMTP_mtpdevice_t *device,
                                 LIBMTP_devicestorage_t *storage,
                                 uint64_t*freespace)
{
    PTPParams *params = (PTPParams*)device->params;

    // Always query the device about this, since some models explicitly
    // needs that. We flush all data on queries storage here.
    if (ptp_operation_issupported(params, PTP_OC_GetStorageInfo))
    {
        PTPStorageInfo storageInfo;
        uint16_t ret;

        ret = ptp_getstorageinfo(params, storage->id, &storageInfo);
        if (ret != PTP_RC_OK)
        {
            add_ptp_error_to_errorstack(device, ret, 
                                        "get_storage_freespace(): could not get storage info.");
            return  - 1;
        }
        if (storage->StorageDescription != NULL)
        {
            kfree(storage->StorageDescription);
        }
        if (storage->VolumeIdentifier != NULL)
        {
            kfree(storage->VolumeIdentifier);
        }
        storage->StorageType = storageInfo.StorageType;
        storage->FilesystemType = storageInfo.FilesystemType;
        storage->AccessCapability = storageInfo.AccessCapability;
        storage->MaxCapacity = storageInfo.MaxCapability;
        storage->FreeSpaceInBytes = storageInfo.FreeSpaceInBytes;
        storage->FreeSpaceInObjects = storageInfo.FreeSpaceInImages;
        storage->StorageDescription = storageInfo.StorageDescription;
        storage->VolumeIdentifier = storageInfo.VolumeLabel;
    }
    if (storage->FreeSpaceInBytes == (uint64_t) - 1)
    {
        return  - 1;
    }
    *freespace = storage->FreeSpaceInBytes;
    return 0;
}

/**
 * This function dumps out a large chunk of textual information
 * provided from the PTP protocol and additionally some extra
 * MTP-specific information where applicable.
 * @param device a pointer to the MTP device to report info from.
 */
void LIBMTP_Dump_Device_Info(LIBMTP_mtpdevice_t *device)
{
    int i;
    PTPParams *params = (PTPParams*)device->params;
    PTP_USB *ptp_usb = (PTP_USB*)device->usbinfo;
    LIBMTP_devicestorage_t *storage = device->storage;
    LIBMTP_device_extension_t *tmpext = device->extensions;

    printk("USB low-level info:\n");
    dump_usbinfo(ptp_usb);
    /* Print out some verbose information */
    printk("Device info:\n");
    printk("   Manufacturer: %s\n", params->deviceinfo.Manufacturer);
    printk("   Model: %s\n", params->deviceinfo.Model);
    printk("   Device version: %s\n", params->deviceinfo.DeviceVersion);
    printk("   Serial number: %s\n", params->deviceinfo.SerialNumber);
    printk("   Vendor extension ID: 0x%08x\n", params
           ->deviceinfo.VendorExtensionID);
    printk("   Vendor extension description: %s\n", params
           ->deviceinfo.VendorExtensionDesc);
    printk("   Detected object size: %d bits\n", device->object_bitsize);
    printk("   Extensions:\n");
    while (tmpext != NULL)
    {
        printk("        %s: %d.%d\n", tmpext->name, tmpext->major, tmpext
               ->minor);
        tmpext = tmpext->next;
    }
    printk("Supported operations:\n");
    for (i = 0; i < params->deviceinfo.OperationsSupported_len; i++)
    {
        char txt[256];

        (void)ptp_render_opcode(params, params
         ->deviceinfo.OperationsSupported[i], sizeof(txt), txt);
        printk("   %04x: %s\n", params->deviceinfo.OperationsSupported[i], txt);
    }
    printk("Events supported:\n");
    if (params->deviceinfo.EventsSupported_len == 0)
    {
        printk("   None.\n");
    }
    else
    {
        for (i = 0; i < params->deviceinfo.EventsSupported_len; i++)
        {
            printk("   0x%04x\n", params->deviceinfo.EventsSupported[i]);
        }
    }
    printk("Device Properties Supported:\n");
    for (i = 0; i < params->deviceinfo.DevicePropertiesSupported_len; i++)
    {
        char const *propdesc = ptp_get_property_description(params, params
            ->deviceinfo.DevicePropertiesSupported[i]);

        if (propdesc != NULL)
        {
            printk("   0x%04x: %s\n", params
                   ->deviceinfo.DevicePropertiesSupported[i], propdesc);
        }
        else
        {
            uint16_t prop = params->deviceinfo.DevicePropertiesSupported[i];
            printk("   0x%04x: Unknown property\n", prop);
        }
    }

    if (ptp_operation_issupported(params, PTP_OC_MTP_GetObjectPropsSupported))
    {
        printk(
               "Playable File (Object) Types and Object Properties Supported:\n");
        for (i = 0; i < params->deviceinfo.ImageFormats_len; i++)
        {
            char txt[256];
            uint16_t ret;
            uint16_t *props = NULL;
            uint32_t propcnt = 0;
            int j;

            (void)ptp_render_ofc(params, params->deviceinfo.ImageFormats[i],
             sizeof(txt), txt);
            printk("   %04x: %s\n", params->deviceinfo.ImageFormats[i], txt);

            ret = ptp_mtp_getobjectpropssupported(params, params
                ->deviceinfo.ImageFormats[i], &propcnt, &props);
            if (ret != PTP_RC_OK)
            {
                add_ptp_error_to_errorstack(device, ret, 
                    "LIBMTP_Dump_Device_Info(): error on query for object properties.");
            }
            else
            {
                for (j = 0; j < propcnt; j++)
                {
                    PTPObjectPropDesc opd;
                    int k;

                    printk("      %04x: %s", props[j],
                           LIBMTP_Get_Property_Description
                           (map_ptp_property_to_libmtp_property(props[j])));
                    // Get a more verbose description
                    ret = ptp_mtp_getobjectpropdesc(params, props[j], params
                        ->deviceinfo.ImageFormats[i], &opd);
                    if (ret != PTP_RC_OK)
                    {
                        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                            "LIBMTP_Dump_Device_Info(): "
                            "could not get property description.");
                        break;
                    }

                    if (opd.DataType == PTP_DTC_STR)
                    {
                        printk(" STRING data type");
                        switch (opd.FormFlag)
                        {
                            case PTP_OPFF_DateTime:
                                printk(" DATETIME FORM");
                                break;
                            case PTP_OPFF_RegularExpression:
                                printk(" REGULAR EXPRESSION FORM");
                                break;
                            case PTP_OPFF_LongString:
                                printk(" LONG STRING FORM");
                                break;
                            default:
                                break;
                        }
                    }
                    else
                    {
                        if (opd.DataType &PTP_DTC_ARRAY_MASK)
                        {
                            printk(" array of");
                        }

                        switch (opd.DataType &(~PTP_DTC_ARRAY_MASK))
                        {

                            case PTP_DTC_UNDEF:
                                printk(" UNDEFINED data type");
                                break;
                            case PTP_DTC_INT8:
                                printk(" INT8 data type");
                                switch (opd.FormFlag)
                                {
                                case PTP_OPFF_Range:
                                    printk(" range: MIN %d, MAX %d, STEP %d",
                                        opd.FORM.Range.MinimumValue.i8,
                                        opd.FORM.Range.MaximumValue.i8,
                                        opd.FORM.Range.StepSize.i8);
                                    break;
                                case PTP_OPFF_Enumeration:
                                    printk(" enumeration: ");
                                    for (k = 0; k <
                                        opd.FORM.Enum.NumberOfValues; k++)
                                    {
                                        printk("%d, ",
                                            opd.FORM.Enum.SupportedValue[k].i8);
                                    }
                                    break;
                                case PTP_OPFF_ByteArray:
                                    printk(" byte array: ");
                                    break;
                                default:
                                    printk(" ANY 8BIT VALUE form");
                                    break;
                                }
                                break;

                            case PTP_DTC_UINT8:
                                printk(" UINT8 data type");
                                switch (opd.FormFlag)
                                {
                                case PTP_OPFF_Range:
                                    printk(" range: MIN %d, MAX %d, STEP %d",
                                        opd.FORM.Range.MinimumValue.u8,
                                        opd.FORM.Range.MaximumValue.u8,
                                        opd.FORM.Range.StepSize.u8);
                                    break;
                                case PTP_OPFF_Enumeration:
                                    printk(" enumeration: ");
                                    for (k = 0; k <
                                        opd.FORM.Enum.NumberOfValues; k++)
                                    {
                                        printk("%d, ",
                                            opd.FORM.Enum.SupportedValue[k].u8);
                                    }
                                    break;
                                case PTP_OPFF_ByteArray:
                                    printk(" byte array: ");
                                    break;
                                default:
                                    printk(" ANY 8BIT VALUE form");
                                    break;
                                }
                                break;

                            case PTP_DTC_INT16:
                                printk(" INT16 data type");
                                switch (opd.FormFlag)
                                {
                                case PTP_OPFF_Range:
                                    printk(" range: MIN %d, MAX %d, STEP %d",
                                        opd.FORM.Range.MinimumValue.i16,
                                        opd.FORM.Range.MaximumValue.i16,
                                        opd.FORM.Range.StepSize.i16);
                                    break;
                                case PTP_OPFF_Enumeration:
                                    printk(" enumeration: ");
                                    for (k = 0; k <
                                        opd.FORM.Enum.NumberOfValues; k++)
                                    {
                                        printk("%d, ",
                                            opd.FORM.Enum.SupportedValue[k].i16)
                                            ;
                                    }
                                    break;
                                default:
                                    printk(" ANY 16BIT VALUE form");
                                    break;
                                }
                                break;

                            case PTP_DTC_UINT16:
                                printk(" UINT16 data type");
                                switch (opd.FormFlag)
                                {
                                case PTP_OPFF_Range:
                                    printk(" range: MIN %d, MAX %d, STEP %d",
                                        opd.FORM.Range.MinimumValue.u16,
                                        opd.FORM.Range.MaximumValue.u16,
                                        opd.FORM.Range.StepSize.u16);
                                    break;
                                case PTP_OPFF_Enumeration:
                                    printk(" enumeration: ");
                                    for (k = 0; k <
                                        opd.FORM.Enum.NumberOfValues; k++)
                                    {
                                        printk("%d, ",
                                            opd.FORM.Enum.SupportedValue[k].u16)
                                            ;
                                    }
                                    break;
                                default:
                                    printk(" ANY 16BIT VALUE form");
                                    break;
                                }
                                break;

                            case PTP_DTC_INT32:
                                printk(" INT32 data type");
                                switch (opd.FormFlag)
                                {
                                case PTP_OPFF_Range:
                                    printk(" range: MIN %d, MAX %d, STEP %d",
                                        opd.FORM.Range.MinimumValue.i32,
                                        opd.FORM.Range.MaximumValue.i32,
                                        opd.FORM.Range.StepSize.i32);
                                    break;
                                case PTP_OPFF_Enumeration:
                                    printk(" enumeration: ");
                                    for (k = 0; k <
                                        opd.FORM.Enum.NumberOfValues; k++)
                                    {
                                        printk("%d, ",
                                            opd.FORM.Enum.SupportedValue[k].i32)
                                            ;
                                    }
                                    break;
                                default:
                                    printk(" ANY 32BIT VALUE form");
                                    break;
                                }
                                break;

                            case PTP_DTC_UINT32:
                                printk(" UINT32 data type");
                                switch (opd.FormFlag)
                                {
                                case PTP_OPFF_Range:
                                    printk(" range: MIN %d, MAX %d, STEP %d",
                                        opd.FORM.Range.MinimumValue.u32,
                                        opd.FORM.Range.MaximumValue.u32,
                                        opd.FORM.Range.StepSize.u32);
                                    break;
                                case PTP_OPFF_Enumeration:
                                    // Special pretty-print for FOURCC codes
                                    if (params->deviceinfo.ImageFormats[i] ==
                                        PTP_OPC_VideoFourCCCodec)
                                    {
                                        printk(
                                            " enumeration of u32 casted FOURCC: ");
                                        for (k = 0; k <
                                            opd.FORM.Enum.NumberOfValues; k++)
                                        {
                                            if 
                                                (opd.FORM.Enum.SupportedValue[k].u32 == 0)
                                            {
                                                printk("ANY, ");
                                            }
                                            else
                                            {
                                                char fourcc[6];
                                                fourcc[0] = 
                                                    (opd.FORM.Enum.SupportedValue[k].u32 >> 24) &0xFFU;
                                                fourcc[1] = 
                                                    (opd.FORM.Enum.SupportedValue[k].u32 >> 16) &0xFFU;
                                                fourcc[2] = 
                                                    (opd.FORM.Enum.SupportedValue[k].u32 >> 8) &0xFFU;
                                                fourcc[3] =
                                                    opd.FORM.Enum.SupportedValue[k].u32 &0xFFU;
                                                fourcc[4] = '\n';
                                                fourcc[5] = '\0';
                                                printk("\"%s\", ", fourcc);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        printk(" enumeration: ");
                                        for (k = 0; k <
                                            opd.FORM.Enum.NumberOfValues; k++)
                                        {
                                            printk("%d, ",
                                                opd.FORM.Enum.SupportedValue[k].u32);
                                        }
                                    }
                                    break;
                                default:
                                    printk(" ANY 32BIT VALUE form");
                                    break;
                                }
                                break;

                            case PTP_DTC_INT64:
                                printk(" INT64 data type");
                                break;

                            case PTP_DTC_UINT64:
                                printk(" UINT64 data type");
                                break;

                            case PTP_DTC_INT128:
                                printk(" INT128 data type");
                                break;

                            case PTP_DTC_UINT128:
                                printk(" UINT128 data type");
                                break;

                            default:
                                printk(" UNKNOWN data type");
                                break;
                        }
                    }
                    if (opd.GetSet)
                    {
                        printk(" GET/SET");
                    }
                    else
                    {
                        printk(" READ ONLY");
                    }
                    printk("\n");
                    ptp_free_objectpropdesc(&opd);
                }
                kfree(props);
            }
        }
    }

    if (storage != NULL && ptp_operation_issupported(params,
        PTP_OC_GetStorageInfo))
    {
        printk("Storage Devices:\n");
        while (storage != NULL)
        {
            printk("   StorageID: 0x%08x\n", storage->id);
            printk("      StorageType: 0x%04x ", storage->StorageType);
            switch (storage->StorageType)
            {
                case PTP_ST_Undefined:
                    printk("(undefined)\n");
                    break;
                case PTP_ST_FixedROM:
                    printk("fixed ROM storage\n");
                    break;
                case PTP_ST_RemovableROM:
                    printk("removable ROM storage\n");
                    break;
                case PTP_ST_FixedRAM:
                    printk("fixed RAM storage\n");
                    break;
                case PTP_ST_RemovableRAM:
                    printk("removable RAM storage\n");
                    break;
                default:
                    printk("UNKNOWN storage\n");
                    break;
            }
            printk("      FilesystemType: 0x%04x ", storage->FilesystemType);
            switch (storage->FilesystemType)
            {
                case PTP_FST_Undefined:
                    printk("(undefined)\n");
                    break;
                case PTP_FST_GenericFlat:
                    printk("generic flat filesystem\n");
                    break;
                case PTP_FST_GenericHierarchical:
                    printk("generic hierarchical\n");
                    break;
                case PTP_FST_DCF:
                    printk("DCF\n");
                    break;
                default:
                    printk("UNKNONWN filesystem type\n");
                    break;
            }
            printk("      AccessCapability: 0x%04x ", storage->AccessCapability)
                   ;
            switch (storage->AccessCapability)
            {
                case PTP_AC_ReadWrite:
                    printk("read/write\n");
                    break;
                case PTP_AC_ReadOnly:
                    printk("read only\n");
                    break;
                case PTP_AC_ReadOnly_with_Object_Deletion:
                    printk("read only + object deletion\n");
                    break;
                default:
                    printk("UNKNOWN access capability\n");
                    break;
            }
            printk("      MaxCapacity: %llu\n", (long long unsigned int)storage
                   ->MaxCapacity);
            printk("      FreeSpaceInBytes: %llu\n", (long long unsigned int)
                   storage->FreeSpaceInBytes);
            printk("      FreeSpaceInObjects: %llu\n", (long long unsigned int)
                   storage->FreeSpaceInObjects);
            printk("      StorageDescription: %s\n", storage
                   ->StorageDescription);
            printk("      VolumeIdentifier: %s\n", storage->VolumeIdentifier);
            storage = storage->next;
        }
    }

    printk("Special directories:\n");
    printk("   Default music folder: 0x%08x\n", device->default_music_folder);
    printk("   Default playlist folder: 0x%08x\n", device
           ->default_playlist_folder);
    printk("   Default picture folder: 0x%08x\n", device
           ->default_picture_folder);
    printk("   Default video folder: 0x%08x\n", device->default_video_folder);
    printk("   Default organizer folder: 0x%08x\n", device
           ->default_organizer_folder);
    printk("   Default zencast folder: 0x%08x\n", device
           ->default_zencast_folder);
    printk("   Default album folder: 0x%08x\n", device->default_album_folder);
    printk("   Default text folder: 0x%08x\n", device->default_text_folder);
}

/**
 * This resets a device in case it supports the <code>PTP_OC_ResetDevice</code>
 * operation code (0x1010).
 * @param device a pointer to the device to reset.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Reset_Device(LIBMTP_mtpdevice_t *device)
{
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;

    if (!ptp_operation_issupported(params, PTP_OC_ResetDevice))
    {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                "LIBMTP_Reset_Device(): device does not support resetting.");
        return  - 1;
    }
    ret = ptp_resetdevice(params);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, "Error resetting.");
        return  - 1;
    }
    return 0;
}

/**
 * This retrieves the manufacturer name of an MTP device.
 * @param device a pointer to the device to get the manufacturer name for.
 * @return a newly allocated UTF-8 string representing the manufacturer name.
 *         The string must be freed by the caller after use. If the call
 *         was unsuccessful this will contain NULL.
 */
char *LIBMTP_Get_Manufacturername(LIBMTP_mtpdevice_t *device)
{
    char *retmanuf = NULL;
    PTPParams *params = (PTPParams*)device->params;

    if (params->deviceinfo.Manufacturer != NULL)
    {
        retmanuf = strdup(params->deviceinfo.Manufacturer);
    }
    return retmanuf;
}

/**
 * This retrieves the model name (often equal to product name)
 * of an MTP device.
 * @param device a pointer to the device to get the model name for.
 * @return a newly allocated UTF-8 string representing the model name.
 *         The string must be freed by the caller after use. If the call
 *         was unsuccessful this will contain NULL.
 */
char *LIBMTP_Get_Modelname(LIBMTP_mtpdevice_t *device)
{
    char *retmodel = NULL;
    PTPParams *params = (PTPParams*)device->params;

    if (params->deviceinfo.Model != NULL)
    {
        retmodel = strdup(params->deviceinfo.Model);
    }
    return retmodel;
}

/**
 * This retrieves the serial number of an MTP device.
 * @param device a pointer to the device to get the serial number for.
 * @return a newly allocated UTF-8 string representing the serial number.
 *         The string must be freed by the caller after use. If the call
 *         was unsuccessful this will contain NULL.
 */
char *LIBMTP_Get_Serialnumber(LIBMTP_mtpdevice_t *device)
{
    char *retnumber = NULL;
    PTPParams *params = (PTPParams*)device->params;

    if (params->deviceinfo.SerialNumber != NULL)
    {
        retnumber = strdup(params->deviceinfo.SerialNumber);
    }
    return retnumber;
}

/**
 * This retrieves the device version (hardware and firmware version) of an
 * MTP device.
 * @param device a pointer to the device to get the device version for.
 * @return a newly allocated UTF-8 string representing the device version.
 *         The string must be freed by the caller after use. If the call
 *         was unsuccessful this will contain NULL.
 */
char *LIBMTP_Get_Deviceversion(LIBMTP_mtpdevice_t *device)
{
    char *retversion = NULL;
    PTPParams *params = (PTPParams*)device->params;

    if (params->deviceinfo.DeviceVersion != NULL)
    {
        retversion = strdup(params->deviceinfo.DeviceVersion);
    }
    return retversion;
}


/**
 * This retrieves the "friendly name" of an MTP device. Usually
 * this is simply the name of the owner or something like
 * "John Doe's Digital Audio Player". This property should be supported
 * by all MTP devices.
 * @param device a pointer to the device to get the friendly name for.
 * @return a newly allocated UTF-8 string representing the friendly name.
 *         The string must be freed by the caller after use.
 * @see LIBMTP_Set_Friendlyname()
 */
char *LIBMTP_Get_Friendlyname(LIBMTP_mtpdevice_t *device)
{
    PTPPropertyValue propval;
    char *retstring = NULL;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;

    if (!ptp_property_issupported(params, PTP_DPC_MTP_DeviceFriendlyName))
    {
        return NULL;
    }

    ret = ptp_getdevicepropvalue(params, PTP_DPC_MTP_DeviceFriendlyName, 
                                 &propval, PTP_DTC_STR);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, "Error getting friendlyname.");
        return NULL;
    }
    if (propval.str != NULL)
    {
        retstring = strdup(propval.str);
        kfree(propval.str);
    }
    return retstring;
}

/**
 * Sets the "friendly name" of an MTP device.
 * @param device a pointer to the device to set the friendly name for.
 * @param friendlyname the new friendly name for the device.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Get_Friendlyname()
 */
int LIBMTP_Set_Friendlyname(LIBMTP_mtpdevice_t *device, char const *const
                            friendlyname)
{
    PTPPropertyValue propval;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;

    if (!ptp_property_issupported(params, PTP_DPC_MTP_DeviceFriendlyName))
    {
        return  - 1;
    }
    propval.str = (char*)friendlyname;
    ret = ptp_setdevicepropvalue(params, PTP_DPC_MTP_DeviceFriendlyName, 
                                 &propval, PTP_DTC_STR);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, "Error setting friendlyname.");
        return  - 1;
    }
    return 0;
}

/**
 * This retrieves the syncronization partner of an MTP device. This
 * property should be supported by all MTP devices.
 * @param device a pointer to the device to get the sync partner for.
 * @return a newly allocated UTF-8 string representing the synchronization
 *         partner. The string must be freed by the caller after use.
 * @see LIBMTP_Set_Syncpartner()
 */
char *LIBMTP_Get_Syncpartner(LIBMTP_mtpdevice_t *device)
{
    PTPPropertyValue propval;
    char *retstring = NULL;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;

    if (!ptp_property_issupported(params, PTP_DPC_MTP_SynchronizationPartner))
    {
        return NULL;
    }

    ret = ptp_getdevicepropvalue(params, PTP_DPC_MTP_SynchronizationPartner, 
                                 &propval, PTP_DTC_STR);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, "Error getting syncpartner.");
        return NULL;
    }
    if (propval.str != NULL)
    {
        retstring = strdup(propval.str);
        kfree(propval.str);
    }
    return retstring;
}


/**
 * Sets the synchronization partner of an MTP device. Note that
 * we have no idea what the effect of setting this to "foobar"
 * may be. But the general idea seems to be to tell which program
 * shall synchronize with this device and tell others to leave
 * it alone.
 * @param device a pointer to the device to set the sync partner for.
 * @param syncpartner the new synchronization partner for the device.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Get_Syncpartner()
 */
int LIBMTP_Set_Syncpartner(LIBMTP_mtpdevice_t *device, char const *const
                           syncpartner)
{
    PTPPropertyValue propval;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;

    if (!ptp_property_issupported(params, PTP_DPC_MTP_SynchronizationPartner))
    {
        return  - 1;
    }
    propval.str = (char*)syncpartner;
    ret = ptp_setdevicepropvalue(params, PTP_DPC_MTP_SynchronizationPartner, 
                                 &propval, PTP_DTC_STR);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, "Error setting syncpartner.");
        return  - 1;
    }
    return 0;
}

/**
 * Checks if the device can stora a file of this size or
 * if it's too big.
 * @param device a pointer to the device.
 * @param filesize the size of the file to check whether it will fit.
 * @param storageid the ID of the storage to try to fit the file on.
 * @return 0 if the file fits, any other value means failure.
 */
static int check_if_file_fits(LIBMTP_mtpdevice_t *device,
                              LIBMTP_devicestorage_t *storage, uint64_t const
                              filesize)
{
    PTPParams *params = (PTPParams*)device->params;
    uint64_t freebytes;
    int ret;

    // If we cannot check the storage, no big deal.
    if (!ptp_operation_issupported(params, PTP_OC_GetStorageInfo))
    {
        return 0;
    }

    ret = get_storage_freespace(device, storage, &freebytes);
    if (ret != 0)
    {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                "check_if_file_fits(): error checking free storage.");
        return  - 1;
    }
    else
    {
        // See if it fits.
        if (filesize > freebytes)
        {
            return  - 1;
        }
    }
    return 0;
}


/**
 * This function retrieves the current battery level on the device.
 * @param device a pointer to the device to get the battery level for.
 * @param maximum_level a pointer to a variable that will hold the
 *        maximum level of the battery if the call was successful.
 * @param current_level a pointer to a variable that will hold the
 *        current level of the battery if the call was successful.
 *        A value of 0 means that the device is on external power.
 * @return 0 if the storage info was successfully retrieved, any other
 *        means failure. A typical cause of failure is that
 *        the device does not support the battery level property.
 */
int LIBMTP_Get_Batterylevel(LIBMTP_mtpdevice_t *device, uint8_t *const
                            maximum_level, uint8_t *const current_level)
{
    PTPPropertyValue propval;
    uint16_t ret;
    PTPParams *params = (PTPParams*)device->params;
    PTP_USB *ptp_usb = (PTP_USB*)device->usbinfo;

    *maximum_level = 0;
    *current_level = 0;

    if (FLAG_BROKEN_BATTERY_LEVEL(ptp_usb) || !ptp_property_issupported(params,
        PTP_DPC_BatteryLevel))
    {
        return  - 1;
    }

    ret = ptp_getdevicepropvalue(params, PTP_DPC_BatteryLevel, &propval,
                                 PTP_DTC_UINT8);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "LIBMTP_Get_Batterylevel(): could not get device property value.");
        return  - 1;
    }

    *maximum_level = device->maximum_battery_level;
    *current_level = propval.u8;

    return 0;
}


/**
 * Formats device storage (if the device supports the operation).
 * WARNING: This WILL delete all data from the device. Make sure you've
 * got confirmation from the user BEFORE you call this function.
 *
 * @param device a pointer to the device containing the storage to format.
 * @param storage the actual storage to format.
 * @return 0 on success, any other value means failure.
 */
int LIBMTP_Format_Storage(LIBMTP_mtpdevice_t *device,
                          LIBMTP_devicestorage_t*storage)
{
    uint16_t ret;
    PTPParams *params = (PTPParams*)device->params;

    if (!ptp_operation_issupported(params, PTP_OC_FormatStore))
    {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                "LIBMTP_Format_Storage(): device does not support formatting storage.");
        return  - 1;
    }
    ret = ptp_formatstore(params, storage->id);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "LIBMTP_Format_Storage(): failed to format storage.");
        return  - 1;
    }
    return 0;
}

/**************************************************************************
 *                                                                        *
 **************************************************************************/

#if 0
    /**
     * Helper function to extract a unicode property off a device.
     * This is the standard way of retrieveing unicode device
     * properties as described by the PTP spec.
     * @param device a pointer to the device to get the property from.
     * @param unicstring a pointer to a pointer that will hold the
     *        property after this call is completed.
     * @param property the property to retrieve.
     * @return 0 on success, any other value means failure.
     */
    static int get_device_unicode_property(LIBMTP_mtpdevice_t *device,
        char**unicstring, uint16_t property)
    {
        PTPPropertyValue propval;
        PTPParams *params = (PTPParams*)device->params;
        uint16_t *tmp;
        uint16_t ret;
        int i;

        if (!ptp_property_issupported(params, property))
        {
            return  - 1;
        }

        // Unicode strings are 16bit unsigned integer arrays.
        ret = ptp_getdevicepropvalue(params, property, &propval,
                                     PTP_DTC_AUINT16);
        if (ret != PTP_RC_OK)
        {
            // TODO: add a note on WHICH property that we failed to get.
            *unicstring = NULL;
            add_ptp_error_to_errorstack(device, ret, 
                                        "get_device_unicode_property(): failed to get unicode property.");
            return  - 1;
        }

        // Extract the actual array.
        // printk("Array of %d elements\n", propval.a.count);
        tmp = kmalloc((propval.a.count + 1) *sizeof(uint16_t), GFP_KERNEL);
        for (i = 0; i < propval.a.count; i++)
        {
            tmp[i] = propval.a.v[i].u16;
            // printk("%04x ", tmp[i]);
        }
        tmp[propval.a.count] = 0x0000U;
        kfree(propval.a.v);

        *unicstring = utf16_to_utf8(device, tmp);

        kfree(tmp);

        return 0;
    }

    /**
     * This function returns the secure time as an XML document string from
     * the device.
     * @param device a pointer to the device to get the secure time for.
     * @param sectime the secure time string as an XML document or NULL if the call
     *         failed or the secure time property is not supported. This string
     *         must be <code>free()</code>:ed by the caller after use.
     * @return 0 on success, any other value means failure.
     */
    int LIBMTP_Get_Secure_Time(LIBMTP_mtpdevice_t *device, char **const sectime)
    {
        return get_device_unicode_property(device, sectime,
            PTP_DPC_MTP_SecureTime);
    }

    /**
     * This function returns the device (public key) certificate as an
     * XML document string from the device.
     * @param device a pointer to the device to get the device certificate for.
     * @param devcert the device certificate as an XML string or NULL if the call
     *        failed or the device certificate property is not supported. This
     *        string must be <code>free()</code>:ed by the caller after use.
     * @return 0 on success, any other value means failure.
     */
    int LIBMTP_Get_Device_Certificate(LIBMTP_mtpdevice_t *device, char **const
                                      devcert)
    {
        return get_device_unicode_property(device, devcert,
            PTP_DPC_MTP_DeviceCertificate);
    }
#endif // #if 0

/**
 * This function retrieves a list of supported file types, i.e. the file
 * types that this device claims it supports, e.g. audio file types that
 * the device can play etc. This list is mitigated to
 * inlcude the file types that libmtp can handle, i.e. it will not list
 * filetypes that libmtp will handle internally like playlists and folders.
 * @param device a pointer to the device to get the filetype capabilities for.
 * @param filetypes a pointer to a pointer that will hold the list of
 *        supported filetypes if the call was successful. This list must
 *        be <code>free()</code>:ed by the caller after use.
 * @param length a pointer to a variable that will hold the length of the
 *        list of supported filetypes if the call was successful.
 * @return 0 on success, any other value means failure.
 * @see LIBMTP_Get_Filetype_Description()
 */
int LIBMTP_Get_Supported_Filetypes(LIBMTP_mtpdevice_t *device, uint16_t **const
                                   filetypes, uint16_t *const length)
{
    PTPParams *params = (PTPParams*)device->params;
    PTP_USB *ptp_usb = (PTP_USB*)device->usbinfo;
    uint16_t *localtypes;
    uint16_t localtypelen;
    uint32_t i;

    // This is more memory than needed if there are unknown types, but what the heck.
    localtypes = (uint16_t*)kmalloc(params->deviceinfo.ImageFormats_len *sizeof
                  (uint16_t), GFP_KERNEL);
    localtypelen = 0;

    for (i = 0; i < params->deviceinfo.ImageFormats_len; i++)
    {
        uint16_t localtype = map_ptp_type_to_libmtp_type(params
            ->deviceinfo.ImageFormats[i]);
        if (localtype != LIBMTP_FILETYPE_UNKNOWN)
        {
            localtypes[localtypelen] = localtype;
            localtypelen++;
        }
    }
    // The forgotten Ogg support on YP-10 and others...
    if (FLAG_OGG_IS_UNKNOWN(ptp_usb))
    {
        localtypes = (uint16_t*)LIBMTP_realloc(localtypes, (params
                      ->deviceinfo.ImageFormats_len + 1) *sizeof(uint16_t),
                      params->deviceinfo.ImageFormats_len *sizeof(uint16_t));
        localtypes[localtypelen] = LIBMTP_FILETYPE_OGG;
        localtypelen++;
    }
    // The forgotten FLAC support on Cowon iAudio S9 and others...
    if (FLAG_FLAC_IS_UNKNOWN(ptp_usb))
    {
        localtypes = (uint16_t*)LIBMTP_realloc(localtypes, (params
                      ->deviceinfo.ImageFormats_len + 1) *sizeof(uint16_t),
                      params->deviceinfo.ImageFormats_len *sizeof(uint16_t));
        localtypes[localtypelen] = LIBMTP_FILETYPE_FLAC;
        localtypelen++;
    }

    *filetypes = localtypes;
    *length = localtypelen;

    return 0;
}

/**
 * This function updates all the storage id's of a device and their
 * properties, then creates a linked list and puts the list head into
 * the device struct. It also optionally sorts this list. If you want
 * to display storage information in your application you should call
 * this function, then dereference the device struct
 * (<code>device-&gt;storage</code>) to get out information on the storage.
 *
 * You need to call this everytime you want to update the
 * <code>device-&gt;storage</code> list, for example anytime you need
 * to check available storage somewhere.
 *
 * <b>WARNING:</b> since this list is dynamically updated, do not
 * reference its fields in external applications by pointer! E.g
 * do not put a reference to any <code>char *</code> field. instead
 * <code>strncpy()</code> it!
 *
 * @param device a pointer to the device to get the storage for.
 * @param sortby an integer that determines the sorting of the storage list.
 *        Valid sort methods are defined in libmtp.h with beginning with
 *        LIBMTP_STORAGE_SORTBY_. 0 or LIBMTP_STORAGE_SORTBY_NOTSORTED to not
 *        sort.
 * @return 0 on success, 1 success but only with storage id's, storage
 *        properities could not be retrieved and -1 means failure.
 */
int LIBMTP_Get_Storage(LIBMTP_mtpdevice_t *device, int const sortby)
{
    uint32_t i = 0;
    PTPStorageInfo storageInfo;
    PTPParams *params = (PTPParams*)device->params;
    PTPStorageIDs storageIDs;
    LIBMTP_devicestorage_t *storage = NULL;
    LIBMTP_devicestorage_t *storageprev = NULL;


    if (device->storage != NULL)
    {
        free_storage_list(device);
    }

    // if (!ptp_operation_issupported(params,PTP_OC_GetStorageIDs))
    //   return -1;
    if (ptp_getstorageids(params, &storageIDs) != PTP_RC_OK)
    {
        return  - 1;
    }
    if (storageIDs.n < 1)
    {
        return  - 1;
    }

    if (!ptp_operation_issupported(params, PTP_OC_GetStorageInfo))
    {
        for (i = 0; i < storageIDs.n; i++)
        {

            storage = (LIBMTP_devicestorage_t*)kmalloc(sizeof
                       (LIBMTP_devicestorage_t), GFP_KERNEL);
            storage->prev = storageprev;
            if (storageprev != NULL)
            {
                storageprev->next = storage;
            }
            if (device->storage == NULL)
            {
                device->storage = storage;
            }

            storage->id = storageIDs.Storage[i];
            storage->StorageType = PTP_ST_Undefined;
            storage->FilesystemType = PTP_FST_Undefined;
            storage->AccessCapability = PTP_AC_ReadWrite;
            storage->MaxCapacity = (uint64_t) - 1;
            storage->FreeSpaceInBytes = (uint64_t) - 1;
            storage->FreeSpaceInObjects = (uint64_t) - 1;
            storage->StorageDescription = strdup("Unknown storage");
            storage->VolumeIdentifier = strdup("Unknown volume");
            storage->next = NULL;

            storageprev = storage;
        }

        device->num_storage = storageIDs.n;
        kfree(storageIDs.Storage);
        return 1;
    }
    else
    {
        for (i = 0; i < storageIDs.n; i++)
        {
            uint16_t ret;
            ret = ptp_getstorageinfo(params, storageIDs.Storage[i], 
                                     &storageInfo);
            if (ret != PTP_RC_OK)
            {
                add_ptp_error_to_errorstack(device, ret, 
                    "LIBMTP_Get_Storage(): Could not get storage info.");
                if (device->storage != NULL)
                {
                    free_storage_list(device);
                }
                return  - 1;
            }

            storage = (LIBMTP_devicestorage_t*)kmalloc(sizeof
                       (LIBMTP_devicestorage_t), GFP_KERNEL);
            storage->prev = storageprev;
            if (storageprev != NULL)
            {
                storageprev->next = storage;
            }
            if (device->storage == NULL)
            {
                device->storage = storage;
            }

            storage->id = storageIDs.Storage[i];
            storage->StorageType = storageInfo.StorageType;
            storage->FilesystemType = storageInfo.FilesystemType;
            storage->AccessCapability = storageInfo.AccessCapability;
            storage->MaxCapacity = storageInfo.MaxCapability;
            storage->FreeSpaceInBytes = storageInfo.FreeSpaceInBytes;
            storage->FreeSpaceInObjects = storageInfo.FreeSpaceInImages;
            storage->StorageDescription = storageInfo.StorageDescription;
            storage->VolumeIdentifier = storageInfo.VolumeLabel;
            storage->next = NULL;

            storageprev = storage;
        }

        if (storage != NULL)
        {
            storage->next = NULL;
        }

        device->num_storage = storageIDs.n;
        sort_storage_by(device, sortby);
        kfree(storageIDs.Storage);
        return 0;
    }
}

/**
 * This creates a new file metadata structure and allocates memory
 * for it. Notice that if you add strings to this structure they
 * will be freed by the corresponding <code>LIBMTP_destroy_file_t</code>
 * operation later, so be careful of using strdup() when assigning
 * strings, e.g.:
 *
 * <pre>
 * LIBMTP_file_t *file = LIBMTP_new_file_t();
 * file->filename = strdup(namestr);
 * ....
 * LIBMTP_destroy_file_t(file);
 * </pre>
 *
 * @return a pointer to the newly allocated metadata structure.
 * @see LIBMTP_destroy_file_t()
 */
LIBMTP_file_t *LIBMTP_new_file_t(void)
{
    LIBMTP_file_t *new = (LIBMTP_file_t*)kmalloc(sizeof(LIBMTP_file_t),
                          GFP_KERNEL);
    if (new == NULL)
    {
        return NULL;
    }
    new->filename = NULL;
    new->item_id = 0;
    new->parent_id = 0;
    new->storage_id = 0;
    new->filesize = 0;
    new->modificationdate = 0;
    new->filetype = LIBMTP_FILETYPE_UNKNOWN;
    new->next = NULL;
    return new;
}

/**
 * This destroys a file metadata structure and deallocates the memory
 * used by it, including any strings. Never use a file metadata
 * structure again after calling this function on it.
 * @param file the file metadata to destroy.
 * @see LIBMTP_new_file_t()
 */
void LIBMTP_destroy_file_t(LIBMTP_file_t *file)
{
    if (file == NULL)
    {
        return ;
    }
    if (file->filename != NULL)
    {
        kfree(file->filename);
    }
    kfree(file);
    return ;
}

#if 0
/**
 * THIS FUNCTION IS DEPRECATED. PLEASE UPDATE YOUR CODE IN ORDER
 * NOT TO USE IT.
 * @see LIBMTP_Get_Filelisting_With_Callback()
 */
LIBMTP_file_t *LIBMTP_Get_Filelisting(LIBMTP_mtpdevice_t *device)
{
    LIBMTP_INFO("WARNING: LIBMTP_Get_Filelisting() is deprecated.\n");
    LIBMTP_INFO(
                "WARNING: please update your code to use LIBMTP_Get_Filelisting_With_Callback()\n");
    return LIBMTP_Get_Filelisting_With_Callback(device, NULL, NULL);
}

/**
 * This returns a long list of all files available
 * on the current MTP device. Folders will not be returned, but abstract
 * entities like playlists and albums will show up as "files". Typical usage:
 *
 * <pre>
 * LIBMTP_file_t *filelist;
 *
 * filelist = LIBMTP_Get_Filelisting_With_Callback(device, callback, data);
 * while (filelist != NULL) {
 *   LIBMTP_file_t *tmp;
 *
 *   // Do something on each element in the list here...
 *   tmp = filelist;
 *   filelist = filelist->next;
 *   LIBMTP_destroy_file_t(tmp);
 * }
 * </pre>
 *
 * If you want to group your file listing by storage (per storage unit) or
 * arrange files into folders, you must dereference the <code>storage_id</code>
 * and/or <code>parent_id</code> field of the returned <code>LIBMTP_file_t</code>
 * struct. To arrange by folders or files you typically have to create the proper
 * trees by calls to <code>LIBMTP_Get_Storage()</code> and/or
 * <code>LIBMTP_Get_Folder_List()</code> first.
 *
 * @param device a pointer to the device to get the file listing for.
 * @param callback a function to be called during the tracklisting retrieveal
 *        for displaying progress bars etc, or NULL if you don't want
 *        any callbacks.
 * @param data a user-defined pointer that is passed along to
 *        the <code>progress</code> function in order to
 *        pass along some user defined data to the progress
 *        updates. If not used, set this to NULL.
 * @return a list of files that can be followed using the <code>next</code>
 *        field of the <code>LIBMTP_file_t</code> data structure.
 *        Each of the metadata tags must be freed after use, and may
 *        contain only partial metadata information, i.e. one or several
 *        fields may be NULL or 0.
 * @see LIBMTP_Get_Filemetadata()
 */
LIBMTP_file_t *LIBMTP_Get_Filelisting_With_Callback(LIBMTP_mtpdevice_t *device,
    LIBMTP_progressfunc_t const callback, void const *const data)
{
    uint32_t i = 0;
    LIBMTP_file_t *retfiles = NULL;
    LIBMTP_file_t *curfile = NULL;
    PTPParams *params = (PTPParams*)device->params;
    PTP_USB *ptp_usb = (PTP_USB*)device->usbinfo;
    uint16_t ret;

    // Get all the handles if we haven't already done that
    if (params->nrofobjects == 0)
    {
        flush_handles(device);
    }

    for (i = 0; i < params->nrofobjects; i++)
    {
        LIBMTP_file_t *file;
        PTPObject *ob,  *xob;

        if (callback != NULL)
        {
            callback(i, params->nrofobjects, data);
        }

        ob = &params->objects[i];

        if (ob->oi.ObjectFormat == PTP_OFC_Association)
        {
            // MTP use this object format for folders which means
            // these "files" will turn up on a folder listing instead.
            continue;
        }

        // Allocate a new file type
        file = LIBMTP_new_file_t();

        file->parent_id = ob->oi.ParentObject;
        file->storage_id = ob->oi.StorageID;

        // This is some sort of unique ID so we can keep track of the track.
        file->item_id = ob->oid;

        // Set the filetype
        file->filetype = map_ptp_type_to_libmtp_type(ob->oi.ObjectFormat);

        // Set the modification date
        file->modificationdate = ob->oi.ModificationDate;

        // Original file-specific properties
        // We only have 32-bit file size here; if we find it, we use the
        // PTP_OPC_ObjectSize property which has 64bit precision.
        file->filesize = ob->oi.ObjectCompressedSize;
        if (ob->oi.Filename != NULL)
        {
            file->filename = strdup(ob->oi.Filename);
        }

        /*
         * A special quirk for devices that doesn't quite
         * remember that some files marked as "unknown" type are
         * actually OGG or FLAC files. We look at the filename extension
         * and see if it happens that this was atleast named "ogg" or "flac"
         * and fall back on this heuristic approach in that case,
         * for these bugged devices only.
         */
        if (file->filetype == LIBMTP_FILETYPE_UNKNOWN)
        {
            if ((FLAG_IRIVER_OGG_ALZHEIMER(ptp_usb) || FLAG_OGG_IS_UNKNOWN
                (ptp_usb)) && has_ogg_extension(file->filename))
            {
                file->filetype = LIBMTP_FILETYPE_OGG;
            }
            if (FLAG_FLAC_IS_UNKNOWN(ptp_usb) && has_flac_extension(file
                ->filename))
            {
                file->filetype = LIBMTP_FILETYPE_FLAC;
            }
        }

        /*
         * If we have a cached, large set of metadata, then use it!
         */
        ret = ptp_object_want(params, ob->oid, PTPOBJECT_MTPPROPLIST_LOADED, 
                              &xob);
        if (ob->mtpprops)
        {
            MTPProperties *prop = ob->mtpprops;
            int i;

            for (i = 0; i < ob->nrofmtpprops; i++)
            {
                // Pick ObjectSize here...
                if (prop->property == PTP_OPC_ObjectSize)
                {
                    if (device->object_bitsize == 64)
                    {
                        file->filesize = prop->propval.u64;
                    }
                    else
                    {
                        file->filesize = prop->propval.u32;
                    }
                    break;
                }
                prop++;
            }
        }
        else
        {
            uint16_t *props = NULL;
            uint32_t propcnt = 0;

            // First see which properties can be retrieved for this object format
            ret = ptp_mtp_getobjectpropssupported(params, ob->oi.ObjectFormat, 
                &propcnt, &props);
            if (ret != PTP_RC_OK)
            {
                add_ptp_error_to_errorstack(device, ret, 
                    "LIBMTP_Get_Filelisting_With_Callback(): call to ptp_mtp_getobjectpropssupported() failed.");
                // Silently fall through.
            }
            else
            {
                int i;
                for (i = 0; i < propcnt; i++)
                {
                    switch (props[i])
                    {
                        case PTP_OPC_ObjectSize:
                            if (device->object_bitsize == 64)
                            {
                                file->filesize = get_u64_from_object(device,
                                    file->item_id, PTP_OPC_ObjectSize, 0);
                            }
                            else
                            {
                                file->filesize = get_u32_from_object(device,
                                    file->item_id, PTP_OPC_ObjectSize, 0);
                            }
                            break;
                        default:
                            break;
                    }
                }
                kfree(props);
            }
        }

        // Add track to a list that will be returned afterwards.
        if (retfiles == NULL)
        {
            retfiles = file;
            curfile = file;
        }
        else
        {
            curfile->next = file;
            curfile = file;
        }

        // Call listing callback
        // double progressPercent = (double)i*(double)100.0 / (double)params->handles.n;

    } // Handle counting loop
    return retfiles;
}

/**
 * This function retrieves the metadata for a single file off
 * the device.
 *
 * Do not call this function repeatedly! The file handles are linearly
 * searched O(n) and the call may involve (slow) USB traffic, so use
 * <code>LIBMTP_Get_Filelisting()</code> and cache the file, preferably
 * as an efficient data structure such as a hash list.
 *
 * Incidentally this function will return metadata for
 * a folder (association) as well, but this is not a proper use
 * of it, it is intended for file manipulation, not folder manipulation.
 *
 * @param device a pointer to the device to get the file metadata from.
 * @param fileid the object ID of the file that you want the metadata for.
 * @return a metadata entry on success or NULL on failure.
 * @see LIBMTP_Get_Filelisting()
 */
LIBMTP_file_t *LIBMTP_Get_Filemetadata(LIBMTP_mtpdevice_t *device, uint32_t
                                       const fileid)
{
    uint32_t i = 0;
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;
    PTPObject *ob;
    LIBMTP_file_t *file;

    // Get all the handles if we haven't already done that
    if (params->nrofobjects == 0)
    {
        flush_handles(device);
    }

    ret = ptp_object_want(params, fileid, PTPOBJECT_OBJECTINFO_LOADED |
                          PTPOBJECT_MTPPROPLIST_LOADED, &ob);
    if (ret != PTP_RC_OK)
    {
        return NULL;
    }

    // Allocate a new file type
    file = LIBMTP_new_file_t();

    file->parent_id = ob->oi.ParentObject;
    file->storage_id = ob->oi.StorageID;

    // Set the filetype
    file->filetype = map_ptp_type_to_libmtp_type(ob->oi.ObjectFormat);

    // Original file-specific properties

    // We only have 32-bit file size here; later we use the PTP_OPC_ObjectSize property
    file->filesize = ob->oi.ObjectCompressedSize;
    if (ob->oi.Filename != NULL)
    {
        file->filename = strdup(ob->oi.Filename);
    }

    // This is some sort of unique ID so we can keep track of the file.
    file->item_id = fileid;

    /*
     * If we have a cached, large set of metadata, then use it!
     */
    if (ob->mtpprops)
    {
        MTPProperties *prop = ob->mtpprops;

        for (i = 0; i < ob->nrofmtpprops; i++, prop++)
        {
            // Pick ObjectSize here...
            if (prop->property == PTP_OPC_ObjectSize)
            {
                // This may already be set, but this 64bit precision value
                // is better than the PTP 32bit value, so let it override.
                if (device->object_bitsize == 64)
                {
                    file->filesize = prop->propval.u64;
                }
                else
                {
                    file->filesize = prop->propval.u32;
                }
                break;
            }
        }
    }
    else
    {
        uint16_t *props = NULL;
        uint32_t propcnt = 0;

        // First see which properties can be retrieved for this object format
        ret = ptp_mtp_getobjectpropssupported(params,
            map_libmtp_type_to_ptp_type(file->filetype), &propcnt, &props);
        if (ret != PTP_RC_OK)
        {
            add_ptp_error_to_errorstack(device, ret, 
                                        "LIBMTP_Get_Filemetadata(): call to ptp_mtp_getobjectpropssupported() failed.");
            // Silently fall through.
        }
        else
        {
            for (i = 0; i < propcnt; i++)
            {
                switch (props[i])
                {
                    case PTP_OPC_ObjectSize:
                        if (device->object_bitsize == 64)
                        {
                            file->filesize = get_u64_from_object(device, file
                                ->item_id, PTP_OPC_ObjectSize, 0);
                        }
                        else
                        {
                            file->filesize = get_u32_from_object(device, file
                                ->item_id, PTP_OPC_ObjectSize, 0);
                        }
                        break;
                    default:
                        break;
                }
            }
            kfree(props);
        }
    }

    return file;
}
#endif // #if 0


/**
 * This is a manual conversion from MTPDataPutFunc to PTPDataPutFunc
 * to isolate the internal type.
 */
static uint16_t put_func_wrapper(PTPParams *params, void *priv, unsigned long
                                 sendlen, unsigned char *data, unsigned
                                 long*putlen)
{
    MTPDataHandler *handler = (MTPDataHandler*)priv;
    uint16_t ret;
    uint32_t local_putlen = 0;
    ret = handler->putfunc(params, handler->priv, sendlen, data, &local_putlen);
    *putlen = local_putlen;
    switch (ret)
    {
        case LIBMTP_HANDLER_RETURN_OK:
            return PTP_RC_OK;
        case LIBMTP_HANDLER_RETURN_ERROR:
            return PTP_ERROR_IO;
        case LIBMTP_HANDLER_RETURN_CANCEL:
            return PTP_ERROR_CANCEL;
        default:
            return PTP_ERROR_IO;
    }
}

/**
 * This creates a new folder structure and allocates memory
 * for it. Notice that if you add strings to this structure they
 * will be freed by the corresponding <code>LIBMTP_folder_track_t</code>
 * operation later, so be careful of using strdup() when assigning
 * strings, e.g.:
 *
 * @return a pointer to the newly allocated folder structure.
 * @see LIBMTP_destroy_folder_t()
 */
LIBMTP_folder_t *LIBMTP_new_folder_t(void)
{
    LIBMTP_folder_t *new = (LIBMTP_folder_t*)kmalloc(sizeof(LIBMTP_folder_t),
                            GFP_KERNEL);
    if (new == NULL)
    {
        return NULL;
    }
    new->folder_id = 0;
    new->parent_id = 0;
    new->storage_id = 0;
    new->name = NULL;
    new->sibling = NULL;
    new->child = NULL;
    return new;
}

/**
 * This recursively deletes the memory for a folder structure.
 * This shall typically be called on a top-level folder list to
 * detsroy the entire folder tree.
 *
 * @param folder folder structure to destroy
 * @see LIBMTP_new_folder_t()
 */
void LIBMTP_destroy_folder_t(LIBMTP_folder_t *folder)
{

    if (folder == NULL)
    {
        return ;
    }

    //Destroy from the bottom up
    if (folder->child != NULL)
    {
        LIBMTP_destroy_folder_t(folder->child);
    }

    if (folder->sibling != NULL)
    {
        LIBMTP_destroy_folder_t(folder->sibling);
    }

    if (folder->name != NULL)
    {
        kfree(folder->name);
    }

    kfree(folder);
}

/**
 * Function used to recursively get subfolders from params.
 */
static LIBMTP_folder_t *get_subfolders_for_folder(LIBMTP_folder_t *list,
    uint32_t parent)
{
    LIBMTP_folder_t *retfolders = NULL;
    LIBMTP_folder_t *children,  *iter,  *curr;

    iter = list->sibling;
    while (iter != list)
    {
        if (iter->parent_id != parent)
        {
            iter = iter->sibling;
            continue;
        }

        /* We know that iter is a child of 'parent', therefore we can safely
         * hold on to 'iter' locally since no one else will steal it
         * from the 'list' as we recurse. */
        children = get_subfolders_for_folder(list, iter->folder_id);

        curr = iter;
        iter = iter->sibling;

        // Remove curr from the list.
        curr->child->sibling = curr->sibling;
        curr->sibling->child = curr->child;

        // Attach the children to curr.
        curr->child = children;

        // Put this folder into the list of siblings.
        curr->sibling = retfolders;
        retfolders = curr;
    }

    return retfolders;
}

#if 0
/**
 * This returns a list of all folders available
 * on the current MTP device.
 *
 * @param device a pointer to the device to get the folder listing for.
 * @return a list of folders
 */
LIBMTP_folder_t *LIBMTP_Get_Folder_List(LIBMTP_mtpdevice_t *device)
{
    PTPParams *params = (PTPParams*)device->params;
    LIBMTP_folder_t head,  *rv;
    int i;

    // Get all the handles if we haven't already done that
    if (params->nrofobjects == 0)
    {
        flush_handles(device);
    }

    /*
     * This creates a temporary list of the folders, this is in a
     * reverse order and uses the Folder pointers that are already
     * in the Folder structure. From this we can then build up the
     * folder hierarchy with only looking at this temporary list,
     * and removing the folders from this temporary list as we go.
     * This significantly reduces the number of operations that we
     * have to do in building the folder hierarchy. Also since the
     * temp list is in reverse order, when we prepend to the sibling
     * list things are in the same order as they were originally
     * in the handle list.
     */
    head.sibling = &head;
    head.child = &head;
    for (i = 0; i < params->nrofobjects; i++)
    {
        LIBMTP_folder_t *folder;
        PTPObject *ob;

        ob = &params->objects[i];
        if (ob->oi.ObjectFormat != PTP_OFC_Association)
        {
            continue;
        }
        /*
         * Do we know how to handle these? They are part
         * of the MTP 1.0 specification paragraph 3.6.4.
         * For AssociationDesc 0x00000001U ptp_mtp_getobjectreferences()
         * should be called on these to get the contained objects, but
         * we basically don't care. Hopefully parent_id is maintained for all
         * children, because we rely on that instead.
         */
        if (ob->oi.AssociationDesc != 0x00000000U)
        {
            LIBMTP_INFO("MTP extended association type 0x%08x encountered\n",
                        ob->oi.AssociationDesc);
        }

        // Create a folder struct...
        folder = LIBMTP_new_folder_t();
        if (folder == NULL)
        {
            // malloc failure or so.
            return NULL;
        }
        folder->folder_id = ob->oid;
        folder->parent_id = ob->oi.ParentObject;
        folder->storage_id = ob->oi.StorageID;
        folder->name = (ob->oi.Filename) ? (char*)strdup(ob->oi.Filename): NULL;

        // pretend sibling says next, and child says prev.
        folder->sibling = head.sibling;
        folder->child = &head;
        head.sibling->child = folder;
        head.sibling = folder;
    }

    // We begin at the root folder and get them all recursively
    rv = get_subfolders_for_folder(&head, 0x00000000U);

    // Some buggy devices may have some files in the "root folder"
    // 0xffffffff so if 0x00000000 didn't return any folders,
    // look for children of the root 0xffffffffU
    if (rv == NULL)
    {
        rv = get_subfolders_for_folder(&head, 0xffffffffU);
        if (rv != NULL)
        {
            LIBMTP_ERROR("Device have files in \"root folder\" 0xffffffffU - "
                         "this is a firmware bug (but continuing)\n");
        }
    }

    // The temp list should be empty. Clean up any orphans just in case.
    while (head.sibling !=  &head)
    {
        LIBMTP_folder_t *curr = head.sibling;

        LIBMTP_INFO("Orphan folder with ID: 0x%08x name: \"%s\" encountered.\n",
                    curr->folder_id, curr->name);
        curr->sibling->child = curr->child;
        curr->child->sibling = curr->sibling;
        curr->child = NULL;
        curr->sibling = NULL;
        LIBMTP_destroy_folder_t(curr);
    }

    return rv;
}
#endif // #if 0


/**
 * This gets a file off the device and calls put_func
 * with chunks of data
 *
 * @param device a pointer to the device to get the file from.
 * @param id the file ID of the file to retrieve.
 * @param put_func the function to call when we have data.
 * @param priv the user-defined pointer that is passed to
 *             <code>put_func</code>.
 * @param callback a progress indicator function or NULL to ignore.
 * @param data a user-defined pointer that is passed along to
 *             the <code>progress</code> function in order to
 *             pass along some user defined data to the progress
 *             updates. If not used, set this to NULL.
 * @return 0 if the transfer was successful, any other value means
 *           failure.
 */
int LIBMTP_Get_File_To_Handler(LIBMTP_mtpdevice_t *device, uint32_t const id,
                               MTPDataPutFunc put_func, void *priv,
                               LIBMTP_progressfunc_t const callback, void
                               const*const data)
{
    PTPObject *ob;
    uint16_t ret;
    PTPParams *params = (PTPParams*)device->params;
    PTP_USB *ptp_usb = (PTP_USB*)device->usbinfo;

    ret = ptp_object_want(params, id, PTPOBJECT_OBJECTINFO_LOADED, &ob);
    if (ret != PTP_RC_OK)
    {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                "LIBMTP_Get_File_To_File_Descriptor(): Could not get object info.");
        return  - 1;
    }
    if (ob->oi.ObjectFormat == PTP_OFC_Association)
    {
        add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                "LIBMTP_Get_File_To_File_Descriptor(): Bad object format.");
        return  - 1;
    }

    // Callbacks
    ptp_usb->callback_active = 1;
    ptp_usb->current_transfer_total = ob->oi.ObjectCompressedSize +
        PTP_USB_BULK_HDR_LEN + sizeof(uint32_t);
    // Request length, one parameter
    ptp_usb->current_transfer_complete = 0;
    ptp_usb->current_transfer_callback = callback;
    ptp_usb->current_transfer_callback_data = data;

    MTPDataHandler mtp_handler;
    mtp_handler.getfunc = NULL;
    mtp_handler.putfunc = put_func;
    mtp_handler.priv = priv;

    PTPDataHandler handler;
    handler.mode = PtpDataHandlerModeNormal;
    handler.getfunc = NULL;
    handler.putfunc = put_func_wrapper;
    handler.priv = &mtp_handler;

    ret = ptp_getobject_to_handler(params, id, &handler);

    ptp_usb->callback_active = 0;
    ptp_usb->current_transfer_callback = NULL;
    ptp_usb->current_transfer_callback_data = NULL;

    if (ret == PTP_ERROR_CANCEL)
    {
        add_error_to_errorstack(device, LIBMTP_ERROR_CANCELLED, 
                                "LIBMTP_Get_File_From_File_Descriptor(): Cancelled transfer.");
        return  - 1;
    }
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "LIBMTP_Get_File_To_File_Descriptor(): Could not get file from device.");
        return  - 1;
    }

    return 0;
}

/**************************************************************************
 *                                                                        *
 **************************************************************************/

#if 0


    /**
     * This function deletes a single file, track, playlist, folder or
     * any other object off the MTP device, identified by the object ID.
     *
     * If you delete a folder, there is no guarantee that the device will
     * really delete all the files that were in that folder, rather it is
     * expected that they will not be deleted, and will turn up in object
     * listings with parent set to a non-existant object ID. The safe way
     * to do this is to recursively delete all files (and folders) contained
     * in the folder, then the folder itself.
     *
     * @param device a pointer to the device to delete the object from.
     * @param object_id the object to delete.
     * @return 0 on success, any other value means failure.
     */
    int LIBMTP_Delete_Object(LIBMTP_mtpdevice_t *device, uint32_t object_id)
    {
        uint16_t ret;
        PTPParams *params = (PTPParams*)device->params;

        ret = ptp_deleteobject(params, object_id, 0);
        if (ret != PTP_RC_OK)
        {
            add_ptp_error_to_errorstack(device, ret, 
                                        "LIBMTP_Delete_Object(): could not delete object.");
            return  - 1;
        }

        return 0;
    }

    /**
     * Internal function to update an object filename property.
     */
    static int set_object_filename(LIBMTP_mtpdevice_t *device, uint32_t
                                   object_id, uint16_t ptp_type, const
                                   char**newname_ptr)
    {
        PTPParams *params = (PTPParams*)device->params;
        PTP_USB *ptp_usb = (PTP_USB*)device->usbinfo;
        PTPObjectPropDesc opd;
        uint16_t ret;
        char *newname;

        // See if we can modify the filename on this kind of files.
        ret = ptp_mtp_getobjectpropdesc(params, PTP_OPC_ObjectFileName,
                                        ptp_type, &opd);
        if (ret != PTP_RC_OK)
        {
            add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                    "set_object_filename(): "
                                    "could not get property description.");
            return  - 1;
        }

        if (!opd.GetSet)
        {
            ptp_free_objectpropdesc(&opd);
            add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                    "set_object_filename(): "
                                    " property is not settable.");
            // TODO: we COULD actually upload/download the object here, if we feel
            //       like wasting time for the user.
            return  - 1;
        }

        newname = strdup(*newname_ptr);

        if (FLAG_ONLY_7BIT_FILENAMES(ptp_usb))
        {
            strip_7bit_from_utf8(newname);
        }

        if (ptp_operation_issupported(params, PTP_OC_MTP_SetObjPropList) &&
            !FLAG_BROKEN_SET_OBJECT_PROPLIST(ptp_usb))
        {
            MTPProperties *props = NULL;
            MTPProperties *prop = NULL;
            int nrofprops = 0;

            prop = ptp_get_new_object_prop_entry(&props,  &nrofprops);
            prop->ObjectHandle = object_id;
            prop->property = PTP_OPC_ObjectFileName;
            prop->datatype = PTP_DTC_STR;
            prop->propval.str = newname;

            ret = ptp_mtp_setobjectproplist(params, props, nrofprops);

            ptp_destroy_object_prop_list(props, nrofprops);

            if (ret != PTP_RC_OK)
            {
                add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                        "set_object_filename(): "
                                        " could not set object property list.");
                ptp_free_objectpropdesc(&opd);
                return  - 1;
            }
        }
        else if (ptp_operation_issupported(params,
                 PTP_OC_MTP_SetObjectPropValue))
        {
            ret = set_object_string(device, object_id, PTP_OPC_ObjectFileName,
                                    newname);
            if (ret != 0)
            {
                add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                        "set_object_filename(): "
                                        " could not set object filename.");
                ptp_free_objectpropdesc(&opd);
                return  - 1;
            }
        }
        else
        {
            kfree(newname);
            add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                    "set_object_filename(): "
                                    " your device doesn't seem to support any known way of setting metadata.");
            ptp_free_objectpropdesc(&opd);
            return  - 1;
        }

        ptp_free_objectpropdesc(&opd);

        // update cached object properties if metadata cache exists
        update_metadata_cache(device, object_id);

        return 0;
    }

    /**
     * This function renames a single file.
     * This simply means that the PTP_OPC_ObjectFileName property
     * is updated, if this is supported by the device.
     *
     * @param device a pointer to the device that contains the file.
     * @param file the file metadata of the file to rename.
     *        On success, the filename member is updated. Be aware, that
     *        this name can be different than newname depending of device restrictions.
     * @param newname the new filename for this object.
     * @return 0 on success, any other value means failure.
     */
    int LIBMTP_Set_File_Name(LIBMTP_mtpdevice_t *device, LIBMTP_file_t *file,
                             const char *newname)
    {
        int ret;

        ret = set_object_filename(device, file->item_id,
                                  map_libmtp_type_to_ptp_type(file->filetype), 
                                  &newname);

        if (ret != 0)
        {
            return ret;
        }

        kfree(file->filename);
        file->filename = strdup(newname);
        return ret;
    }

    /**
     * This function renames a single folder.
     * This simply means that the PTP_OPC_ObjectFileName property
     * is updated, if this is supported by the device.
     *
     * @param device a pointer to the device that contains the file.
     * @param folder the folder metadata of the folder to rename.
     *        On success, the name member is updated. Be aware, that
     *        this name can be different than newname depending of device restrictions.
     * @param newname the new name for this object.
     * @return 0 on success, any other value means failure.
     */
    int LIBMTP_Set_Folder_Name(LIBMTP_mtpdevice_t *device, LIBMTP_folder_t
                               *folder, const char *newname)
    {
        int ret;

        ret = set_object_filename(device, folder->folder_id,
                                  PTP_OFC_Association, &newname);

        if (ret != 0)
        {
            return ret;
        }

        kfree(folder->name);
        folder->name = strdup(newname);
        return ret;
    }

    /**
     * This function renames a single track.
     * This simply means that the PTP_OPC_ObjectFileName property
     * is updated, if this is supported by the device.
     *
     * @param device a pointer to the device that contains the file.
     * @param track the track metadata of the track to rename.
     *        On success, the filename member is updated. Be aware, that
     *        this name can be different than newname depending of device restrictions.
     * @param newname the new filename for this object.
     * @return 0 on success, any other value means failure.
     */
    int LIBMTP_Set_Track_Name(LIBMTP_mtpdevice_t *device, LIBMTP_track_t *track,
                              const char *newname)
    {
        int ret;

        ret = set_object_filename(device, track->item_id,
                                  map_libmtp_type_to_ptp_type(track->filetype),
                                  &newname);

        if (ret != 0)
        {
            return ret;
        }

        kfree(track->filename);
        track->filename = strdup(newname);
        return ret;
    }

    /**
     * This function renames a single playlist object file holder.
     * This simply means that the <code>PTP_OPC_ObjectFileName</code>
     * property is updated, if this is supported by the device.
     * The playlist filename should nominally end with an extension
     * like ".pla".
     *
     * NOTE: if you want to change the metadata the device display
     * about a playlist you must <i>not</i> use this function,
     * use <code>LIBMTP_Update_Playlist()</code> instead!
     *
     * @param device a pointer to the device that contains the file.
     * @param playlist the playlist metadata of the playlist to rename.
     *        On success, the name member is updated. Be aware, that
     *        this name can be different than newname depending of device restrictions.
     * @param newname the new name for this object.
     * @return 0 on success, any other value means failure.
     * @see LIBMTP_Update_Playlist()
     */
    int LIBMTP_Set_Playlist_Name(LIBMTP_mtpdevice_t *device, LIBMTP_playlist_t
                                 *playlist, const char *newname)
    {
        int ret;

        ret = set_object_filename(device, playlist->playlist_id,
                                  PTP_OFC_MTP_AbstractAudioVideoPlaylist,
                                  &newname);

        if (ret != 0)
        {
            return ret;
        }

        kfree(playlist->name);
        playlist->name = strdup(newname);
        return ret;
    }

    /**
     * This function renames a single album.
     * This simply means that the <code>PTP_OPC_ObjectFileName</code>
     * property is updated, if this is supported by the device.
     * The album filename should nominally end with an extension
     * like ".alb".
     *
     * NOTE: if you want to change the metadata the device display
     * about a playlist you must <i>not</i> use this function,
     * use <code>LIBMTP_Update_Album()</code> instead!
     *
     * @param device a pointer to the device that contains the file.
     * @param album the album metadata of the album to rename.
     *        On success, the name member is updated. Be aware, that
     *        this name can be different than newname depending of device restrictions.
     * @param newname the new name for this object.
     * @return 0 on success, any other value means failure.
     * @see LIBMTP_Update_Album()
     */
    int LIBMTP_Set_Album_Name(LIBMTP_mtpdevice_t *device, LIBMTP_album_t *album,
                              const char *newname)
    {
        int ret;

        ret = set_object_filename(device, album->album_id,
                                  PTP_OFC_MTP_AbstractAudioAlbum, &newname);

        if (ret != 0)
        {
            return ret;
        }

        kfree(album->name);
        album->name = strdup(newname);
        return ret;
    }

    /**
     * THIS FUNCTION IS DEPRECATED. PLEASE UPDATE YOUR CODE IN ORDER
     * NOT TO USE IT.
     *
     * @see LIBMTP_Set_File_Name()
     * @see LIBMTP_Set_Track_Name()
     * @see LIBMTP_Set_Folder_Name()
     * @see LIBMTP_Set_Playlist_Name()
     * @see LIBMTP_Set_Album_Name()
     */
    int LIBMTP_Set_Object_Filename(LIBMTP_mtpdevice_t *device, uint32_t
                                   object_id, char *newname)
    {
        int ret;
        LIBMTP_file_t *file;

        file = LIBMTP_Get_Filemetadata(device, object_id);

        if (file == NULL)
        {
            add_error_to_errorstack(device, LIBMTP_ERROR_GENERAL, 
                                    "LIBMTP_Set_Object_Filename(): "
                                    "could not get file metadata for target object.");
            return  - 1;
        }

        ret = set_object_filename(device, object_id,
                                  map_libmtp_type_to_ptp_type(file->filetype), 
                                  (const char **) &newname);

        kfree(file);

        return ret;
    }
#endif // #if 0

/**
 * Helper function. Returns a folder structure for a
 * specified id.
 *
 * @param folderlist list of folders to search
 * @id id of folder to look for
 * @return a folder or NULL if not found
 */
LIBMTP_folder_t *LIBMTP_Find_Folder(LIBMTP_folder_t *folderlist, uint32_t id)
{
    LIBMTP_folder_t *ret = NULL;

    if (folderlist == NULL)
    {
        return NULL;
    }

    if (folderlist->folder_id == id)
    {
        return folderlist;
    }

    if (folderlist->sibling)
    {
        ret = LIBMTP_Find_Folder(folderlist->sibling, id);
    }

    if (folderlist->child && ret == NULL)
    {
        ret = LIBMTP_Find_Folder(folderlist->child, id);
    }

    return ret;
}


/**
 * This create a folder on the current MTP device. The PTP name
 * for a folder is "association". The PTP/MTP devices does not
 * have an internal "folder" concept really, it contains a flat
 * list of all files and some file are "associations" that other
 * files and folders may refer to as its "parent".
 *
 * @param device a pointer to the device to create the folder on.
 * @param name the name of the new folder. Note this can be modified
 *        if the device does not support all the characters in the
 *        name.
 * @param parent_id id of parent folder to add the new folder to,
 *        or 0 to put it in the root directory.
 * @param storage_id id of the storage to add this new folder to.
 *        notice that you cannot mismatch storage id and parent id:
 *        they must both be on the same storage! Pass in 0 if you
 *        want to create this folder on the default storage.
 * @return id to new folder or 0 if an error occured
 */
uint32_t LIBMTP_Create_Folder(LIBMTP_mtpdevice_t *device, char*name, uint32_t
                              parent_id, uint32_t storage_id)
{
    PTPParams *params = (PTPParams*)device->params;
    PTP_USB *ptp_usb = (PTP_USB*)device->usbinfo;
    uint32_t parenthandle = 0;
    uint32_t store;
    PTPObjectInfo new_folder;
    uint16_t ret;
    uint32_t new_id = 0;

    if (storage_id == 0)
    {
        // I'm just guessing that a folder may require 512 bytes
        store = get_writeable_storageid(device, 512);
    }
    else
    {
        store = storage_id;
    }
    parenthandle = parent_id;

    memset(&new_folder, 0, sizeof(new_folder));
    new_folder.Filename = name;
    if (FLAG_ONLY_7BIT_FILENAMES(ptp_usb))
    {
        strip_7bit_from_utf8(new_folder.Filename);
    }
    new_folder.ObjectCompressedSize = 1;
    new_folder.ObjectFormat = PTP_OFC_Association;
    new_folder.ProtectionStatus = PTP_PS_NoProtection;
    new_folder.AssociationType = PTP_AT_GenericFolder;
    new_folder.ParentObject = parent_id;
    new_folder.StorageID = store;

    // Create the object
    // FIXME: use send list here if available.
    ret = ptp_sendobjectinfo(params, &store, &parenthandle,  &new_id,
                             &new_folder);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "LIBMTP_Create_Folder: Could not send object info.");
        if (ret == PTP_RC_AccessDenied)
        {
            add_ptp_error_to_errorstack(device, ret, "ACCESS DENIED.");
        }
        return 0;
    }
    // NOTE: don't destroy the new_folder objectinfo, because it is statically referencing
    // several strings.

    add_object_to_cache(device, new_id);

    return new_id;
}


/**
 * Dummy function needed to interface to upstream
 * ptp.c/ptp.h files.
 */
void ptp_nikon_getptpipguid(unsigned char *guid)
{
    return ;
}

/**
 * Add an object to cache.
 * @param device the device which may have a cache to which the object should be added.
 * @param object_id the object to add to the cache.
 */
static void add_object_to_cache(LIBMTP_mtpdevice_t *device, uint32_t object_id)
{
    PTPParams *params = (PTPParams*)device->params;
    uint16_t ret;

    ret = ptp_add_object_to_cache(params, object_id);
    if (ret != PTP_RC_OK)
    {
        add_ptp_error_to_errorstack(device, ret, 
                                    "add_object_to_cache(): couldn't add object to cache");
    }
}


/**
 * Update cache after object has been modified
 * @param device the device which may have a cache to which the object should be updated.
 * @param object_id the object to update.
 */
/* 
static void update_metadata_cache(LIBMTP_mtpdevice_t *device, uint32_t object_id)
{
PTPParams *params = (PTPParams *)device->params;

ptp_remove_object_from_cache(params, object_id);
add_object_to_cache(device, object_id);
}
 */

// --------------------------------------------------------------------------------------------------
/*
void LIBMTP_Flush_Handles(LIBMTP_mtpdevice_t *device)
{
    flush_handles(device);
}
*/

/**************************************************************************
 *                                                                        *
 **************************************************************************/

void LIBMTP_Add_Error_To_ErrorStack(LIBMTP_mtpdevice_t *device,
                                    LIBMTP_error_number_t errornumber, char
                                    const *const error_text)
{
    add_error_to_errorstack(device, errornumber, error_text);
}

/**************************************************************************
 *                                                                        *
 **************************************************************************/

void LIBMTP_Parse_Extension_Descriptor(LIBMTP_mtpdevice_t*mtpdevice, char *desc)
{
    parse_extension_descriptor(mtpdevice, desc);
}

// --------------------------------------------------------------------------------------------------
