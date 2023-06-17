/*****************************************************************************
 * libasf_guid.h :
 *****************************************************************************/

#ifndef LIBASG_GUID_H
#define LIBASG_GUID_H

#include <vlc_codecs.h>

/*****************************************************************************
 * Structure needed for decoder
 *****************************************************************************/
enum
{
    ASF_OBJECT_NULL = 0,
    ASF_OBJECT_ROOT,
    ASF_OBJECT_HEADER,
    ASF_OBJECT_DATA,
    ASF_OBJECT_INDEX,
    ASF_OBJECT_FILE_PROPERTIES,
    ASF_OBJECT_STREAM_PROPERTIES,
    ASF_OBJECT_HEADER_EXTENSION,
    ASF_OBJECT_CODEC_LIST,
    ASF_OBJECT_MARKER,
    ASF_OBJECT_CONTENT_DESCRIPTION,
    ASF_OBJECT_METADATA,
    ASF_OBJECT_PADDING,
    ASF_OBJECT_OTHER,
};

static const guid_t asf_object_null_guid =
{
    0x00000000,
    0x0000,
    0x0000,
    { 0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00 }
};

static const guid_t vlc_object_root_guid =
{
    0x00000000,
    0x0000,
    0x0000,
    { 0x56, 0x4C, 0x43, 0x52, 0x4F, 0x4F, 0x54, 0x00 }
};

/* Top-Level object */
static const guid_t asf_object_header_guid =
{0x75B22630, 0x668E, 0x11CF, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const guid_t asf_object_data_guid =
{0x75B22636, 0x668E, 0x11CF, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const guid_t asf_object_simple_index_guid =
{0x33000890, 0xE5B1, 0x11CF, {0x89, 0xF4, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB}};

static const guid_t asf_object_index_guid =
{0xD6E229D3, 0x35DA, 0x11D1, {0x90, 0x34, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xBE}};

/* Header object */
static const guid_t asf_object_file_properties_guid =
{0x8cabdca1, 0xa947, 0x11cf, {0x8e, 0xe4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const guid_t asf_object_stream_properties_guid =
{0xB7DC0791, 0xA9B7, 0x11CF, {0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const guid_t asf_object_header_extension_guid =
{0x5FBF03B5, 0xA92E, 0x11CF, {0x8E, 0xE3, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const guid_t asf_object_codec_list_guid =
{0x86D15240, 0x311D, 0x11D0, {0xA3, 0xA4, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6}};

static const guid_t asf_object_marker_guid =
{0xF487CD01, 0xA951, 0x11CF, {0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const guid_t asf_object_content_description_guid =
{0x75B22633, 0x668E, 0x11CF, {0xa6, 0xd9, 0x00, 0xaa, 0x00, 0x62, 0xce, 0x6c}};

static const guid_t asf_object_extended_content_description =
{0xD2D0A440, 0xE307, 0x11D2, {0x97, 0xF0, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50}};

static const guid_t asf_object_padding =
{0x1806D474, 0xCADF, 0x4509, {0xA4, 0xBA, 0x9A, 0xAB, 0xCB, 0x96, 0xAA, 0xE8}};

/* Header Extension object */
static const guid_t asf_object_advanced_mutual_exclusion =
{0xA08649CF, 0x4775, 0x4670, {0x8A, 0x16, 0x6E, 0x35, 0x35, 0x75, 0x66, 0xCD}};

static const guid_t asf_object_stream_prioritization =
{0xD4FED15B, 0x88D3, 0x454F, {0x81, 0xF0, 0xED, 0x5C, 0x45, 0x99, 0x9E, 0x24}};

static const guid_t asf_object_metadata_guid =
{0xC5F8CBEA, 0x5BAF, 0x4877, {0x84, 0x67, 0xAA, 0x8C, 0x44, 0xFA, 0x4C, 0xCA}};

/* Stream Properties object */
static const guid_t asf_object_stream_type_audio =
{0xF8699E40, 0x5B4D, 0x11CF, {0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B}};

static const guid_t asf_object_stream_type_video =
{0xbc19efc0, 0x5B4D, 0x11CF, {0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B}};

static const guid_t asf_object_stream_type_command =
{0x59DACFC0, 0x59E6, 0x11D0, {0xA3, 0xAC, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6}};

static const guid_t asf_object_stream_type_binary =
{0x3AFB65E2, 0x47EF, 0x40F2, {0xAC, 0x2C, 0x70, 0xA9, 0x0D, 0x71, 0xD3, 0x43}};

/* TODO */
// top-level
static const guid_t asf_object_media_index_guid =
{0xFEB103F8, 0x12AD, 0x4C64, {0x84, 0x0F, 0x2A, 0x1D, 0x2F, 0x7A, 0xD4, 0x8C}};

static const guid_t asf_object_timecode_index_guid =
{0x3CB73FD0, 0x0C4A, 0x4803, {0x95, 0x3D, 0xED, 0xF7, 0xB6, 0x22, 0x8F, 0x0C}};

// header
static const guid_t asf_object_script_command_guid =
{0x1EFB1A30, 0x0B62, 0x11D0, {0xA3, 0x9B, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6}};

static const guid_t asf_object_bitrate_mutual_exclusion_guid =
{0xD6E229DC, 0x35DA, 0x11D1, {0x90, 0x34, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xBE}};

static const guid_t asf_object_error_correction_guid =
{0x75B22635, 0x668E, 0x11CF, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const guid_t asf_object_content_branding_guid =
{0x2211B3FA, 0xBD23, 0x11D2, {0xB4, 0xB7, 0x00, 0xA0, 0xC9, 0x55, 0xFC, 0x6E}};

static const guid_t asf_object_stream_bitrate_properties =
{0x7BF875CE, 0x468D, 0x11D1, {0x8D, 0x82, 0x00, 0x60, 0x97, 0xC9, 0xA2, 0xB2}};

static const guid_t asf_object_content_encryption_guid =
{0x2211B3FB, 0xBD23, 0x11D2, {0xB4, 0xB7, 0x00, 0xA0, 0xC9, 0x55, 0xFC, 0x6E}};

static const guid_t asf_object_extended_content_encryption_guid =
{0x298AE614, 0x2622, 0x4C17, {0xB9, 0x35, 0xDA, 0xE0, 0x7E, 0xE9, 0x28, 0x9C}};

static const guid_t asf_object_digital_signature_guid =
{0x2211B3FC, 0xBD23, 0x11D2, {0xB4, 0xB7, 0x00, 0xA0, 0xC9, 0x55, 0xFC, 0x6E}};

// header extension
static const guid_t asf_object_extended_stream_properties_guid =
{0x14E6A5CB, 0xC672, 0x4332, {0x83, 0x99, 0xA9, 0x69, 0x52, 0x06, 0x5B, 0x5A}};

static const guid_t asf_object_group_mutual_exclusion_guid =
{0xD1465A40, 0x5A79, 0x4338, {0xB7, 0x1B, 0xE3, 0x6B, 0x8F, 0xD6, 0xC2, 0x49}};

static const guid_t asf_object_bandwidth_sharing_guid =
{0xA69609E6, 0x517B, 0x11D2, {0xB6, 0xAF, 0x00, 0xC0, 0x4F, 0xD9, 0x08, 0xE9}};

static const guid_t asf_object_language_list =
{0x7C4346A9, 0xEFE0, 0x4BFC, {0xB2, 0x29, 0x39, 0x3E, 0xDE, 0x41, 0x5C, 0x85}};

static const guid_t asf_object_metadata_library_guid =
{0x44231C94, 0x9498, 0x49D1, {0xA1, 0x41, 0x1D, 0x13, 0x4E, 0x45, 0x70, 0x54}};

static const guid_t asf_object_index_parameters_guid =
{0xD6E229DF, 0x35DA, 0x11D1, {0x90, 0x34, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xBE}};

static const guid_t asf_object_media_object_index_parameters_guid =
{0x6B203BAD, 0x3F11, 0x48E4, {0xAC, 0xA8, 0xD7, 0x61, 0x3D, 0xE2, 0xCF, 0xA7}};

static const guid_t asf_object_timecode_index_parameters_guid =
{0xF55E496D, 0x9797, 0x4B5D, {0x8C, 0x8B, 0x60, 0x4D, 0xFE, 0x9B, 0xFB, 0x24}};

static const guid_t asf_object_compatibility_guid =
{0x26F18B5D, 0x4584, 0x47EC, {0x9F, 0x5F, 0x0E, 0x65, 0x1F, 0x04, 0x52, 0xC9}};

static const guid_t asf_object_advanced_content_encryption_guid =
{0x43058533, 0x6981, 0x49E6, {0x9B, 0x74, 0xAD, 0x12, 0xCB, 0x86, 0xD5, 0x8C}};

//
static const guid_t asf_object_extended_stream_type_audio =
{0x31178C9D, 0x03E1, 0x4528, {0xB5, 0x82, 0x3D, 0xF9, 0xDB, 0x22, 0xF5, 0x03}};

static const guid_t asf_guid_reserved_1 =
{0xABD3D211, 0xA9BA, 0x11cf, {0x8E, 0xE6, 0x00, 0xC0, 0x0C ,0x20, 0x53, 0x65}};

static const guid_t asf_guid_reserved_2 = //object_codec_list_reserved_guid =
{0x86D15241, 0x311D, 0x11D0, {0xA3, 0xA4, 0x00, 0xA0, 0xC9, 0x03, 0x48, 0xF6}};

// Stream Properties Object Error Correction
static const guid_t asf_no_error_correction_guid =
{0x20FB5700, 0x5B55, 0x11CF, {0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B}};

static const guid_t asf_guid_audio_conceal_spread =
{0xBFC3CD50, 0x618F, 0x11CF, {0x8B, 0xB2, 0x00, 0xAA, 0x00, 0xB4, 0xE2, 0x20}};

// Mutual exclusion
static const guid_t asf_guid_mutex_language =
{0xD6E22A00, 0x35DA, 0x11D1, {0x90, 0x34, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xBE}};

static const guid_t asf_guid_mutex_bitrate =
{0xD6E22A01, 0x35DA, 0x11D1, {0x90, 0x34, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xBE}};

static const guid_t asf_guid_mutex_unknown =
{0xD6E22A02, 0x35DA, 0x11D1, {0x90, 0x34, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xBE}};

// Obscure ones
// http://cpan.uwinnipeg.ca/htdocs/Audio-WMA/Audio/WMA.pm.html

static const guid_t nonasf_object_index_placeholder_guid =
{0xD9AADE20, 0x7C17, 0x4F9C, {0xBC, 0x28, 0x85, 0x55, 0xDD, 0x98, 0xE2, 0xA2}};

static const guid_t nonasf_object_compatibility =
{0x26F18B5D, 0x4584, 0x47EC, {0x9F, 0x5F, 0x0E, 0x65, 0x1F, 0x04, 0x52, 0xC9}};

// MS foundations Payload Extensions for non compressed payloads
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd757606%28v=vs.85%29.aspx

static const guid_t mfasf_sampleextension_sampleduration_guid =
{0xC6BD9450, 0x867F, 0x4907, {0x83, 0xA3, 0xC7, 0x79, 0x21, 0xB7, 0x33, 0xAD}};

static const guid_t mfasf_sampleextension_outputcleanpoint_guid =
{0xF72A3C6F, 0x6EB4, 0x4EBC, {0xB1, 0x92, 0x09, 0xAD, 0x97, 0x59, 0xE8, 0x28}};

static const guid_t mfasf_sampleextension_smtpe_guid =
{0x399595EC, 0x8667, 0x4E2D, {0x8F, 0xDB, 0x98, 0x81, 0x4C, 0xE7, 0x6C, 0x1E}};

static const guid_t mfasf_sampleextension_filename_guid =
{0xE165EC0E, 0x19ED, 0x45D7, {0xB4, 0xA7, 0x25, 0xCB, 0xD1, 0xE2, 0x8E, 0x9B}};

static const guid_t mfasf_sampleextension_contenttype_guid =
{0xD590DC20, 0x07BC, 0x436C, {0x9C, 0xF7, 0xF3, 0xBB, 0xFB, 0xF1, 0xA4, 0xDC}};

static const guid_t mfasf_sampleextension_pixelaspectratio_guid =
{0x1B1EE554, 0xF9EA, 0x4BC8, {0x82, 0x1A, 0x37, 0x6B, 0x74, 0xE4, 0xC4, 0xB8}};

static const guid_t mfasf_sampleextension_encryptionsampleid_guid =
{0x6698B84E, 0x0AFA, 0x4330, {0xAE, 0xB2, 0x1C, 0x0A, 0x98, 0xD7, 0xA4, 0x4D}};

static const guid_t mfasf_sampleextension_encryptionkeyid_guid =
{0x76376591, 0x795F, 0x4DA1, {0x86, 0xED, 0x9D, 0x46, 0xEC, 0xA1, 0x09, 0x49}};

// DVR ones

static const guid_t asf_dvr_sampleextension_videoframe_guid =
{0xDD6432CC, 0xE229, 0x40DB, {0x80, 0xF6, 0xD2, 0x63, 0x28, 0xD2, 0x76, 0x1F}};

static const guid_t asf_dvr_sampleextension_timing_rep_data_guid =
{0xFD3CC02A, 0x06DB, 0x4CFA, {0x80, 0x1C, 0x72, 0x12, 0xd3, 0x87, 0x45, 0xE4}};

/****************************************************************************
 * GUID functions
 ****************************************************************************/
static inline void ASF_GetGUID( guid_t *p_guid, const uint8_t *p_data )
{
    p_guid->Data1 = GetDWLE( p_data );
    p_guid->Data2 = GetWLE( p_data + 4);
    p_guid->Data3 = GetWLE( p_data + 6);
    memcpy( p_guid->Data4, p_data + 8, 8 );
}

#endif
