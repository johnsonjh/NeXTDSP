
/*
 *	accesssound.h
 *	Copyright 1988-89 NeXT, Inc.
 *
 */

#import "soundstruct.h"
#import "sounderror.h"
#import <cthreads.h>

#define SND_ACCESS_OUT 1
#define SND_ACCESS_DSP 2
#define SND_ACCESS_IN 4

typedef struct {
    int priority;
    int duration;
} SNDNegotiationData;

typedef int (*SNDNegotiationFun)(void *arg, SNDNegotiationData *data);

#define NULL_NEGOTIATION_FUN ((SNDNegotiationFun)0)

int SNDAcquire(int access_code, int priority, int preempt, int timeout,
		SNDNegotiationFun negotiation_function, void *arg,
		port_t *device_port, port_t *owner_port);
int SNDReset(int access_code, port_t dev_port, port_t owner_port);
int SNDRelease(int access_code, port_t device_port, port_t owner_port);
/*
 * Acquire/release the specified resources. Acquiring a resources makes
 * it active, such that other acquisition requests may fail (even in the
 * requests are in the same process). These calls should bracket any use
 * of the resources. These functions are automatically called by
 * SNDStartPlaying and SNDStartRecording. The SNDReset function causes the
 * owned resources to be reset to the state that they were when acquired.
 * If access is granted, another attempt at access will cause the
 * negotiation function to be called. Returning a value of zero indicates
 * that access is being released and for the caller to try again. If a
 * null function is provided, ownership of the resources is absolute.
 */
 
int SNDReserve(int access_code, int priority);
/*
 * Establishes exclusive use of specified resources until SNDUnreserve
 * is called or the process terminates. If this routine is not called,
 * then SNDStartPlaying or SNDStartRecording will obtain access and release it 
 * automatically. The access_code determines which physical resources are
 * to be reserved, and the priority is used to resolve conflicts.
 * Use of this routine is optional; if used, it must eventually be followed by 
 * a call to SNDUnreserve.
 * An error code is returned if the resources cannot be reserved.
 */

int SNDUnreserve(int access_code);
/*
 * Frees up sound resources for other processes to use.
 * An error code is returned if the sound is not already reserved.
 */

int SNDSetHost(char *newHostname);
/*
 * Chooses the host by name for subsequent playback or recording. Passing
 * NULL or a zero length string restores the default, which is the local
 * host. If sound is currently recording or playing, or if the sound resources
 * are reserved, then the host cannot be changed.
 * an error code is returned.
 */

int SNDBootDSP(port_t device_port, port_t owner_port, SNDSoundStruct *dspCore);
/*
 * Boots the dsp specified by the device_port and owner_port (obtained via
 * SNDAcquire). The boot image is specified in an SNDSoundStruct, which can
 * be created by SNDReadDSPfile (see filesound.h).
 * This routine uses a bootstrap downloader, making possible the loading
 * of all internal and external RAM of the DSP except for the top 6 words
 * of external ram.
 */


