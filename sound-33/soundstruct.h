/*
 *	soundstruct.h
 *	Copyright 1988-89 NeXT, Inc.
 *
 * SNDSoundStruct - This data format for sound is used as the soundfile
 * format, and also the "NeXT sound pasteboard type". It consists of a header
 * and two variable length quantities: textual information (the info string)
 * and raw data. The raw data starts past the info; the dataLocation is
 * normally used to specify an offset from the beginning of the SNDSoundStruct
 * structure to this data. The dataSize is the length of the raw data in bytes. 
 * The dataFormat, samplingRate, and channelCount further describe the data.
 * The data itself may be anything; the format determines what the data
 * actually means (i.e. sample data, dsp core structure).
 * The magic number value may be used to determine the byte order of the data.
 * The info string is any null-terminated data that the application may need
 * (i.e. copyright information, textual description). The four bytes allocated
 * are a minimum; the info string may extend any length beyond the structure.
 */
typedef struct {
    int magic;		/* must be equal to SND_MAGIC */
    int dataLocation;	/* Offset or pointer to the raw data */
    int dataSize;	/* Number of bytes of data in the raw data */
    int dataFormat;	/* The data format code */
    int samplingRate;	/* The sampling rate */
    int channelCount;	/* The number of channels */
    char info[4];	/* Textual information relating to the sound. */
} SNDSoundStruct;


/*
 * The magic number must appear at the beginning of every SNDSoundStruct.
 * It is used for type checking and byte ordering information.
 */
#define SND_MAGIC ((int)0x2e736e64)

/*
 * NeXT data format codes. User-defined formats should be greater than 255.
 * Negative format numbers are reserved.
 */
#define SND_FORMAT_UNSPECIFIED		(0)
#define SND_FORMAT_MULAW_8		(1)
#define SND_FORMAT_LINEAR_8		(2)
#define SND_FORMAT_LINEAR_16		(3)
#define SND_FORMAT_LINEAR_24		(4)
#define SND_FORMAT_LINEAR_32		(5)
#define SND_FORMAT_FLOAT		(6)
#define SND_FORMAT_DOUBLE		(7)
#define SND_FORMAT_INDIRECT		(8)
#define SND_FORMAT_NESTED		(9)
#define SND_FORMAT_DSP_CORE		(10)
#define SND_FORMAT_DSP_DATA_8		(11)
#define SND_FORMAT_DSP_DATA_16		(12)
#define SND_FORMAT_DSP_DATA_24		(13)
#define SND_FORMAT_DSP_DATA_32		(14)
#define SND_FORMAT_DISPLAY		(16)
#define SND_FORMAT_MULAW_SQUELCH	(17)
#define	SND_FORMAT_EMPHASIZED		(18)
#define	SND_FORMAT_COMPRESSED		(19)
#define	SND_FORMAT_COMPRESSED_EMPHASIZED (20)
#define	SND_FORMAT_DSP_COMMANDS		(21)
#define	SND_FORMAT_DSP_COMMANDS_SAMPLES	(22)

/*
 * Sampling rates directly supported in hardware.
 */
#define SND_RATE_CODEC		(8012.8210513)
#define SND_RATE_LOW		(22050.0)
#define SND_RATE_HIGH		(44100.0)

