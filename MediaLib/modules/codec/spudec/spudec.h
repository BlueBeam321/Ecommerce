/*****************************************************************************
 * spudec.h : sub picture unit decoder thread interface
 *****************************************************************************/

/* #define DEBUG_SPUDEC 1 */

struct decoder_sys_t
{
    bool b_packetizer;
    bool b_disabletrans;

    mtime_t i_pts;
    unsigned int i_spu_size;
    unsigned int i_rle_size;
    unsigned int i_spu;

    block_t *p_block;

    /* We will never overflow */
    uint8_t buffer[65536];
};

/*****************************************************************************
 * Amount of bytes we GetChunk() in one go
 *****************************************************************************/
#define SPU_CHUNK_SIZE              0x200

/*****************************************************************************
 * SPU commands
 *****************************************************************************/
#define SPU_CMD_FORCE_DISPLAY       0x00
#define SPU_CMD_START_DISPLAY       0x01
#define SPU_CMD_STOP_DISPLAY        0x02
#define SPU_CMD_SET_PALETTE         0x03
#define SPU_CMD_SET_ALPHACHANNEL    0x04
#define SPU_CMD_SET_COORDINATES     0x05
#define SPU_CMD_SET_OFFSETS         0x06
#define SPU_CMD_END                 0xff

/*****************************************************************************
 * Prototypes
 *****************************************************************************/
subpicture_t * ParsePacket( decoder_t * );
