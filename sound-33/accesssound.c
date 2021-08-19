#ifdef SHLIB
#include "shlib.h"
#endif SHLIB

/*
 *	accesssound.c - negotiated access to the sound and dsp resources.
 *	Written by Lee Boynton
 *	Copyright 1988-89 NeXT, Inc.
 *
 *	Modification History:
 *	04/11/90/mtm	Added #import <stdlib.h> per OS request.
 *	05/15/90/mtm	Try bootstrap_lookup() first on local machine.
 *	06/07/90/mtm	Check for valid device port in SNDAcquire().
 *	07/24/90/mtm	Make sure bootstrap_port is valid.
 *	08/07/90/mtm	bootstrap_lookup() -> bootstrap_look_up().
 *	09/18/90/mtm	Check for bad port in SNDAcquire() (bug #8684).
 *	09/21/90/mtm	Re-init on device port lookup error and 
 *			try open/ioctl (bug #8308).
 */

#import <mach.h>
#import <libc.h>
#import <sys/file.h>
#import <sys/ioctl.h>
#import <nextdev/snd_msgs.h>
#import <stdlib.h>
#import <cthreads.h>
#import <servers/netname.h>
#import <servers/bootstrap.h>
#import <string.h>
#import "sounderror.h"
#import "sounddriver.h"
#import "filesound.h"
#import "accesssound.h"


/*
 * Miscellaneous local things
 */
static int open_sound(port_t *device_port)
{
#define OPEN_TIMEOUT 10000
    int fd, r;
    msg_header_t msg;

    if ((fd = open("/dev/sound", O_RDONLY, 0)) < 0)
	return SND_ERR_CANNOT_OPEN;

    if (ioctl(fd, SOUNDIOCDEVPORT, (int)thread_reply()) < 0)
	return SND_ERR_CANNOT_OPEN;
    
    msg.msg_local_port = thread_reply();
    msg.msg_size = sizeof(msg);
    
    r = msg_receive(&msg, RCV_TIMEOUT, OPEN_TIMEOUT);
    if (r != RCV_SUCCESS)
	return SND_ERR_CANNOT_OPEN;
    
    if (msg.msg_id != SND_MSG_RET_DEVICE)
	return SND_ERR_CANNOT_OPEN;

    *device_port = msg.msg_remote_port;
    return SND_ERR_NONE;
}

static kern_return_t snddriver_get_device_port (
	char		*hostname,		// name of device's host
	port_t		*device_port)		// returned device port
/*
 * Get the sound driver device port.  Try the bootstrap server first if
 * hostname is "" (local machine).  This gives you a secure port that
 * can't be yanked away from a different machine.
 * Try open() and ioctl() as a last resort.  This will work for root jobs.
 */
{
    port_t dev_port;
    int err;
    port_set_name_t enabled;
    int num_msgs, backlog;
    boolean_t owner, receiver;

    if (strlen(hostname) == 0) {
	/*
	 * Get the bootstrap_port if it has gone away.
	 */
	err = port_status(task_self(), bootstrap_port, &enabled, &num_msgs,
			  &backlog, &owner, &receiver);
	if (err)
	    err = task_get_bootstrap_port(task_self(), &bootstrap_port);
	if (!err)
	    err = bootstrap_look_up(bootstrap_port,"sound",&dev_port);
	if (err)
	    err = netname_look_up(name_server_port,hostname,"sound",&dev_port);
	if (err)
	    err = open_sound(&dev_port);
    } else
	err = netname_look_up(name_server_port,hostname,"sound",&dev_port);
    if (err)
	*device_port = PORT_NULL;
    else
	*device_port = dev_port;
    return err;
}


/*
 *
 * A device table is maintained in every process, keeping track of that 
 * process's local connections to any number of devices (there is one device
 * per host). Processes compete to acquire access to the physical resources.
 * Each device record maintains the current local owners and priorities of
 * the three resources (sound out, sound in, and dsp) for each device. If the
 * resource has no owner recorded in the record, then the resource may be owned
 * be any other process (or none at all).
 *
 */

typedef struct _device_record_t {
    char *hostname;		/* host on which this device resides */
    port_t device_port;		/* the device port */
    port_t sndout_owner;	/* the owner of the sound out resource */
    port_t sndin_owner;		/* the owner of the sound in resource */
    port_t dsp_owner;		/* the owner of the dsp resource */
    SNDNegotiationFun sndout_negot_fun;	/* called to negotiate sndout*/
    SNDNegotiationFun sndin_negot_fun;	/* called to negotiate sndout */
    SNDNegotiationFun dsp_negot_fun;	/* called to negotiate sndout */
    SNDNegotiationData *dsp_negot_dat;	/* argument for negot_fun */
    int sndout_priority;	/* the priority of the sound out ownership */
    int sndin_priority;		/* the priority of the sound in ownership */
    int dsp_priority;		/* the priority of the dsp ownership */
    int reserved_access;	/* the reserved access of the device */
    int reserved_priority;	/* the reserved priority of the device */
    int active_access;		/* the active access of the device */
    int extra;
} device_record_t;

#define NULL_DEVICE_RECORD ((device_record_t *)0)

 static device_record_t *device_table=0;
 static int device_count=0, device_max = 0;

static device_record_t *get_device_record(port_t a_port)
/*
 * Return the device record associated with the given port. Note that
 * only one device may have a given port as one or more of its components
 * (in other words, devices have a one-to-one mapping with device records,
 * and a port can only own resources on one device at a time).
 */
{
    int i;
    for (i=0;i<device_count;i++)
	if ( (device_table[i].device_port == a_port) ||
	     (device_table[i].sndout_owner == a_port) ||
	     (device_table[i].sndin_owner == a_port) ||
	     (device_table[i].dsp_owner == a_port) )
	    return &device_table[i];
    return NULL_DEVICE_RECORD;
}

static device_record_t *get_device_for_host(char *hostname)
/*
 * Return the device record for the sound device on the named host.
 */
{
    port_t dev_port;
//    char *host = (char *)NXUniqueString(hostname? hostname : "");
    char *h, *host = hostname? hostname : "";
    int i, err;
    for (i=0;i<device_count;i++)
	if (!strcmp(device_table[i].hostname,host))
	    return &device_table[i];
    err = snddriver_get_device_port(host,&dev_port);
    if (err) return NULL_DEVICE_RECORD;
    if (device_count == device_max) {
	int i;
	device_record_t *old_device_table = device_table;
	device_max = device_max? 2*device_max : 1;
	device_table = (device_record_t *)
				malloc(device_max*sizeof(device_record_t));
	if (!device_table) {
	    device_table = old_device_table;
	    return NULL_DEVICE_RECORD;
	}
	if (old_device_table) {
	    for (i=0; i<device_count; i++)
		device_table[i] = old_device_table[i];
	    free(old_device_table);
	}
    }
    h = (char *)malloc(strlen(host)+1);
    strcpy(h,host);
    device_table[device_count].hostname = h;
    device_table[device_count].device_port = dev_port;
    device_table[device_count].sndout_owner = PORT_NULL;
    device_table[device_count].sndin_owner = PORT_NULL;
    device_table[device_count].dsp_owner = PORT_NULL;
    device_table[device_count].sndout_priority = -1;
    device_table[device_count].sndin_priority = -1;
    device_table[device_count].dsp_priority = -1;
    device_table[device_count].reserved_access = 0;
    device_table[device_count].reserved_priority = -1;
    device_table[device_count].active_access = 0;
    return &device_table[device_count++];
}


/***********************************
 *
 */

static mutex_t device_lock=0;
static device_record_t *current_device=0;

static int initialize()
{
    static int initialized=0;

    if (initialized) return SND_ERR_NONE;
    current_device = get_device_for_host("");
    if (!current_device) return SND_ERR_CANNOT_ACCESS;
    device_lock = mutex_alloc();
    mutex_init(device_lock);
    initialized = 1;
    return SND_ERR_NONE;
}


static int reset_access(device_record_t *device, int access_code)
{
    int err;
    port_t a_port, dev_port = device->device_port;

    if ((access_code & SND_ACCESS_DSP) && device->dsp_owner) {
	a_port = device->dsp_owner;
 	err = snddriver_set_dsp_owner_port(dev_port,a_port,&a_port);
	if (err)  goto error_exit;
    }
    if ((access_code & SND_ACCESS_OUT) && device->sndout_owner) {
	a_port = device->sndout_owner;
 	err = snddriver_set_sndout_owner_port(dev_port, a_port, &a_port);
	if (err) goto error_exit;
    }
    if ((access_code & SND_ACCESS_IN) && device->sndin_owner) {
	a_port = device->sndin_owner;
 	err = snddriver_set_sndin_owner_port(dev_port, a_port, &a_port);
	if (err)  goto error_exit;
    }
    return SND_ERR_NONE;
 error_exit:
     if (access_code & SND_ACCESS_DSP)
	device->sndout_owner = PORT_NULL;
     if (access_code & SND_ACCESS_IN)
	device->sndout_owner = PORT_NULL;
    if (access_code & SND_ACCESS_OUT)
	device->sndout_owner = PORT_NULL;
    return SND_ERR_KERNEL;
}

static int release_access(device_record_t *device, int access_code)
{
    int err = SND_ERR_NONE;
    port_t old_sndout_port, old_sndin_port, old_dsp_port;

    if ((access_code & SND_ACCESS_DSP) && device->dsp_owner) {
	old_dsp_port = device->dsp_owner;
	device->dsp_owner = PORT_NULL;
    } else 
	old_dsp_port = PORT_NULL;
    if ((access_code & SND_ACCESS_OUT) && device->sndout_owner) {
	if (device->sndout_owner != old_dsp_port)
	    old_sndout_port = device->sndout_owner;
	else
	    old_sndout_port = PORT_NULL;
	device->sndout_owner = PORT_NULL;
    } else
	old_sndout_port = PORT_NULL;
    if ((access_code & SND_ACCESS_IN) && device->sndin_owner) {
	if ( (device->sndin_owner != old_dsp_port) &&
	     (device->sndin_owner != old_sndout_port) )
	    old_sndin_port = device->sndin_owner;
	else
	    old_sndin_port = PORT_NULL;
	device->sndin_owner = PORT_NULL;
    } else
	old_sndin_port = PORT_NULL;
    if (old_dsp_port)
	err = port_deallocate(task_self(), old_dsp_port);
    if (old_sndout_port)
	err |= port_deallocate(task_self(), old_sndout_port);
    if (old_sndin_port)
	err |= port_deallocate(task_self(), old_sndin_port);
    return err? SND_ERR_KERNEL : SND_ERR_NONE;
}

static int negotiate_access(device_record_t *device,
			    int access_code,
			    int priority,
			    int preempt,
			    port_t negotiation_port,
			    port_t new_owner_port)
{
    return -1;
}

static int acquire_remote_access(device_record_t *device,
				 int access_code,
				 int priority,
				 int preempt,
				 port_t owner_port)
/*
 * Acquire access of the specified resources, none of which are currently
 * owned by this process. If any resource cannot be acquired, an error is 
 * returned, and the current state is left clean.
 */
{
    int err;
    port_t dev_port = device->device_port, a_port;

    if (access_code & SND_ACCESS_DSP) {
	a_port = owner_port;
	err = snddriver_set_dsp_owner_port(dev_port, a_port, &a_port);
	if (err) {
	    if (a_port != owner_port) {
		err = negotiate_access(device, SND_ACCESS_DSP, priority,
						 preempt, a_port, owner_port);
		if (err) goto error3;
	    } else 
		goto error3;
	}
	device->dsp_owner = owner_port;
    }
    if (access_code & SND_ACCESS_OUT) {
	a_port = owner_port;
	err = snddriver_set_sndout_owner_port(dev_port,a_port,&a_port);
	if (err) {
	    if (a_port != owner_port) {
		err = negotiate_access(device, SND_ACCESS_OUT, priority, 
						preempt, a_port, owner_port);
		if (err) goto error1;
	    } else goto error1;
	}
	device->sndout_owner = owner_port;
    }
    if (access_code & SND_ACCESS_IN) {
	a_port = owner_port;
	err = snddriver_set_sndin_owner_port(dev_port,a_port,&a_port);
	if (err) {
	    if (a_port != owner_port) {
		err = negotiate_access(device, SND_ACCESS_IN, priority, 
						preempt,a_port, owner_port);
		if (err) goto error2;
	    } else goto error2;
	}
	device->sndin_owner = owner_port;
    }
    return SND_ERR_NONE;
 error2:
    if (access_code & SND_ACCESS_IN)
	device->sndin_owner = PORT_NULL;
 error1:
    if (access_code & SND_ACCESS_OUT)
	device->sndout_owner = PORT_NULL;
 error3:
    if (access_code & SND_ACCESS_DSP)
	device->dsp_owner = PORT_NULL;
    return err;
}

static int acquire_access( device_record_t *device,
			      int access_code,
			      int priority,
			      int preempt,
			      int timeout, //currently ignored
			      port_t *owner_port)
{
    int err, local_access, remote_access, pri = priority + (preempt? 1 : 0);
    port_t local_owner;
    int local_owner_is_new;

    if (!access_code) return SND_ERR_NONE;

    /* check for local conflicts and get an owner port */
    local_access = 0;
    local_owner = PORT_NULL;
    if ((access_code & SND_ACCESS_OUT) && device->sndout_owner) {
	if ((device->active_access & SND_ACCESS_OUT) &&
					(pri <= device->sndout_priority))
	    return SND_ERR_CANNOT_ACCESS;
	else {
	    local_access |= SND_ACCESS_OUT;
	    local_owner = device->sndout_owner;
	}
    }
    if ((access_code & SND_ACCESS_IN) && device->sndin_owner) {
	if (pri <= device->sndin_priority)
	    return SND_ERR_CANNOT_ACCESS;
	else if (local_owner) {
	    if (device->sndin_owner != local_owner)
		return SND_ERR_CANNOT_ACCESS;
	    else
		local_access |= SND_ACCESS_IN;
	} else {
	    local_access |= SND_ACCESS_IN;
	    local_owner = device->sndin_owner;
	}
    }
    if ((access_code & SND_ACCESS_DSP) && device->dsp_owner) {
	if (pri <= device->dsp_priority) {
	    return SND_ERR_CANNOT_ACCESS;
	} else if (local_owner) {
	    if (device->dsp_owner != local_owner)
		return SND_ERR_CANNOT_ACCESS;
	    else
		local_access |= SND_ACCESS_DSP;
	} else {
	    local_access |= SND_ACCESS_DSP;
	    local_owner = device->dsp_owner;
	}
    }
    if (!local_owner) {
	err = port_allocate(task_self(), &local_owner);
	if (err) return SND_ERR_CANNOT_ACCESS;
	local_owner_is_new = 1;
    } else
	local_owner_is_new = 0;
    
    /* check for remote conflicts (grabbing resources if possible) */
    remote_access = access_code & ~local_access;
    if (remote_access) {
	err = acquire_remote_access(device,remote_access,
					priority,preempt,local_owner);
	if (err) {
	    if (local_owner_is_new)
		err = port_deallocate(task_self(), local_owner);
	    return SND_ERR_CANNOT_ACCESS;
	}
    }

    /* all resources available -- reset ones in use locally */
    if (local_access) {
	err = reset_access(device,local_access);
	if (err) {
	    release_access(device,access_code);
	    return err;
	}
    }

    *owner_port = local_owner;
    return SND_ERR_NONE;
}

static int isValidOwner(device_record_t *device,
				int access_code,port_t owner_port)
{
    if (access_code & SND_ACCESS_DSP && device->sndout_owner != owner_port)
	return 0;
    if (access_code & SND_ACCESS_IN && device->sndout_owner != owner_port)
	return 0;
    if (access_code & SND_ACCESS_OUT && device->sndout_owner != owner_port)
	return 0;
    return 1;
}


/***********************************
 *
 * Exported functions
 *
 */

int SNDAcquire(int access_code, int priority, int preempt, int timeout,
		SNDNegotiationFun negotiation_function, void *arg,
		port_t *device_port, port_t *owner_port)
{
    port_t a_port;
    int err = initialize();
    device_record_t *device;
    port_set_name_t enabled;
    int num_msgs, backlog;
    boolean_t owner, receiver;

    if (err) return err;
    mutex_lock(device_lock);
    if (device_port && *device_port) {
	device = get_device_record(*device_port);
	if (device == NULL_DEVICE_RECORD) {
	    mutex_unlock(device_lock);
	    return SND_ERR_CANNOT_ACCESS;
	}
    } else
	device = current_device;
    /*
     * If the device has been reset, grab a new device port.
     */
    err = port_status(task_self(), device->device_port, &enabled, &num_msgs,
		      &backlog, &owner, &receiver);
    if (err) {
	err = snddriver_get_device_port(device->hostname, &device->device_port);
	if (err) {
	    mutex_unlock(device_lock);
	    return SND_ERR_CANNOT_ACCESS;
	}
	device->sndout_owner = PORT_NULL;
	device->sndin_owner = PORT_NULL;
	device->dsp_owner = PORT_NULL;
    }
    if (!access_code) {
	*device_port = device->device_port;
	*owner_port = PORT_NULL;
	mutex_unlock(device_lock);
	return SND_ERR_NONE;
    }
    err = acquire_access(current_device,access_code,priority,preempt,
			 timeout, &a_port);
    if (!err) {
	if (access_code & SND_ACCESS_OUT) {
	    if (!(device->reserved_access & SND_ACCESS_OUT) || 
	    				device->reserved_priority < priority)
		device->sndout_priority = priority;
	    else
		device->sndout_priority = device->reserved_priority;
	    device->sndout_negot_fun = negotiation_function;
	}
	if (access_code & SND_ACCESS_IN) {
	    if (!(device->reserved_access & SND_ACCESS_IN) || 
	    				device->reserved_priority < priority)
		device->sndin_priority = priority;
	    else
		device->sndin_priority = device->reserved_priority;
	    device->sndin_negot_fun = negotiation_function;
	}
	if (access_code & SND_ACCESS_DSP) {
	    if (!(device->reserved_access & SND_ACCESS_DSP) || 
	    				device->reserved_priority < priority)
		device->dsp_priority = priority;
	    else
		device->dsp_priority = device->reserved_priority;
	    device->dsp_negot_fun = negotiation_function;
	}
	current_device->active_access |= access_code;
	*device_port = current_device->device_port;
	*owner_port = a_port;
    }
    mutex_unlock(device_lock);
    return err;
}

int SNDReset(int access_code, port_t dev_port, port_t owner_port)
{
    int err = initialize();
    device_record_t *device;
    if (err) return err;
    mutex_lock(device_lock);
    device = get_device_record(dev_port);
    if (!device)
	err = SND_ERR_UNKNOWN;
    else if (isValidOwner(device,access_code,owner_port))
	err = reset_access(device,access_code);
    else
	err = SND_ERR_CANNOT_ACCESS;
    mutex_unlock(device_lock);
    return err;
}

int SNDRelease(int access_code, port_t dev_port, port_t owner_port)
{
    device_record_t *device;
    int releasable_access, err = initialize();
    if (err) return err;
    mutex_lock(device_lock);
    device = get_device_record(dev_port);
    if (!device)
	err = SND_ERR_UNKNOWN;
    else {
	releasable_access = access_code & ~device->reserved_access;
	err = release_access(device, releasable_access);
    }
    if (!err) {
	device->active_access &= ~access_code;
	if (access_code & SND_ACCESS_OUT)
	    device->sndout_priority = -1;
	if (access_code & SND_ACCESS_IN)
	    device->sndin_priority = -1;
	if (access_code & SND_ACCESS_DSP)
	    device->dsp_priority = -1;
    }
    mutex_unlock(device_lock);
    return err;
}

int SNDReserve(int access_code, int priority)
{
    port_t a_port;
    int total_access, err = initialize();
    if (err) return err;
    mutex_lock(device_lock);
    total_access = access_code | current_device->reserved_access;
    err = acquire_access(current_device,total_access,priority,TRUE,-1,&a_port);
    if (!err) {
	current_device->reserved_access = total_access;
	current_device->reserved_priority = priority;
    }
    mutex_unlock(device_lock);
    return err;
}

int SNDUnreserve(int access_code)
{
    int err = initialize();
    if (err) return err;
    mutex_lock(device_lock);
    err = release_access(current_device, access_code);
    current_device->reserved_access &= ~access_code;
    mutex_unlock(device_lock);
    return err;
}

int SNDSetHost(char *newHostname)
{
    int err = initialize();
    device_record_t *dev;
    if (err) return err;
    dev = get_device_for_host(newHostname);
    if (!dev) return SND_ERR_CANNOT_ACCESS;
    current_device = dev;
    return SND_ERR_NONE;
}

#define DSP_SPACE_P 4

static int simple_boot_image_offset(int *dspImage, int imageSize)
{
    int i=0, *image = dspImage;
    while (i<imageSize) {
	if ((image[i] == DSP_SPACE_P) && (image[i+1] == 0))
	    return i+3;
	if ((image[i] > 0) && (image[i] < 5))
	    return -1; // not simple
	i += 2;
	i += (image[i] + 1);
     }
     return -1;
}

int SNDBootDSP(port_t dev_port, port_t owner_port, SNDSoundStruct *dspCore)
{
    port_t cmd_port;
    int err, temp, dspSize, *dspImage;

    err = snddriver_get_dsp_cmd_port(dev_port, owner_port, &cmd_port);
    if (err != KERN_SUCCESS) return err;
    dspSize = dspCore->dataSize>>2;
    dspImage = (int *)((int)dspCore + dspCore->dataLocation);
    temp = simple_boot_image_offset(dspImage,dspSize);
    if (temp > 0) {
	dspImage += temp;
	dspSize -= temp;
	err = snddriver_dsp_boot(cmd_port,dspImage,dspSize,
						SNDDRIVER_MED_PRIORITY);
    } else {
	static int booterSize=0, *booterImage=0;
	if (!booterImage) {
	    SNDSoundStruct *s;
	    err = SNDReadSoundfile("/usr/lib/sound/booter.snd", &s);
	    if (err != SND_ERR_NONE) return err;
	    booterSize = (s->dataSize>>2) - 3;
	    booterImage = (int *)((int)s + s->dataLocation + (3*4));
	}
	err = snddriver_dsp_boot(cmd_port,booterImage,booterSize,
						SNDDRIVER_MED_PRIORITY);
	if (err != KERN_SUCCESS) return err;
	err = snddriver_dsp_write(cmd_port, (void *)dspImage,
				    		dspSize,sizeof(int), 
						    SNDDRIVER_MED_PRIORITY);
    }
    return err;
}



