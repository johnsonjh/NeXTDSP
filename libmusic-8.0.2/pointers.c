/*
 * This file contains the pointers to symbols not defined in libmusic but used
 * by the code in libmusic.  For the pointers who's symbols are expected to be
 * in a shared library they are initialized to the addresses in that library
 * (for routines this is the address of a branch table slot not the actual
 * address of the routine).
 *
 * New pointers are to be added at the end of this file before the symbol 
 * _libmusic_pointers_pad and the pad to be reduced by the number of pointers
 * added.  This keeps the global data size of this file constant and thus does
 * not effect the addresses of other symbols.
 */
/* DON'T FORGET TO REDUCE THE PAD WHEN YOU ADD A FUNCTION. 
 * total size (pad + functions) == 512 * sizeof(void (*))
 * 
 * POINTERS MUST BE ADDED AT THE END OF THE FILE!!!
 */

/* Modification history:
        05/05/90 - Daj added DSPMKSetTime() and 
                   DSPMKClearDSPSoundOutBufferTimed(). Removed DSPMKWriteLong()
                   and DSPMemoryClear(). 
        05/06/90 - Daj added DSPGetLowestExternalUserAddress(),
                   DSPGetHighestExternalUserAddress(), 
                   DSPMKGetClipCountAddress(),
  5/12/90/jos - added several math.h routines (sin,cos,exp,sqrt,log,log10,fabs)
  5/13/90/daj - Decremented pad count by 7 for new functions that JOS added. 
                Also reordered the math.h functions that they're in order.
  6/10/90/daj - Added DSPMKPauseOrchestra. Pad is now 326
  7/25/90/daj  - Added the following libsys symbols that are needed
		   by _MKSprintf:  isnan, isinf, _dbltopdfp
		   Also added libsys symbols: cthread_fork, cthread_yield,
		   cthread_set_errno_self, thread_info, thread_priority. 
		   Pad is now 318 (8 functions added)
   07/26/90 - Added libsys symbols:
                   mutex_try_lock, mutex_wait_lock, msg_send, port_set_add,
		   port_set_remove, port_set_allocate, thread_self,
                   ur_cthread_self, NXVPrintf. Pad is now 309 (9 functions 
		   added)
   08/01/90 - Changed order to be compatible with libmusic-3
   08/03/90 - Added _fsdfsi and bcopy -- compiler needs these. Pad is now 307
   08/03/90 - Removed _dbltopdfp. This is safe because lib hasn't been
   	      released yet. Pad is now 308
   08/13/90 - Added libsys functions: thread_policy, 
                   processor_set_default, host_processor_set_priv,
                   processor_set_policy_enable
                   processor_set_policy_disable
                   geteuid, host_self, host_priv_self. 
		   8 functions added. 
		   Pad is now 300

  08/30/90  - Added zone malloc function NXDefaultMallocZone().  Pad is 299.
  09/28/90  - Added cthread_abort(), condition_wait(), cond_signal(). 
              Added objc_getClassWithoutWarning, cthread_detach. Pad 294
*/


/* libmidi -- not a shlib so pointers are left NULL. They're filled in
              by linker. */
void * _libmusic_midi_clear_queue = (void *)0x00000000;
void * _libmusic_midi_error_string = (void *)0x00000000;
void * _libmusic_midi_get_data = (void *)0x00000000;
void * _libmusic_midi_get_in_timer_port = (void *)0x00000000;
void * _libmusic_midi_get_recv = (void *)0x00000000;
void * _libmusic_midi_get_xmit = (void *)0x00000000;
void * _libmusic_midi_output_queue_notify = (void *)0x00000000;
void * _libmusic_midi_reply_handler = (void *)0x00000000;
void * _libmusic_midi_send_raw_data = (void *)0x00000000;
void * _libmusic_midi_set_owner = (void *)0x00000000;
void * _libmusic_midi_set_proto = (void *)0x00000000;
void * _libmusic_midi_set_sys_ignores = (void *)0x00000000;
void * _libmusic_midi_timer_error_string = (void *)0x00000000;
void * _libmusic_timer_set = (void *)0x00000000;
void * _libmusic_timer_start = (void *)0x00000000;
void * _libmusic_timer_stop = (void *)0x00000000;

/* libdsp */
void * _libmusic_DSPClose = (void *)0x040920d8;
void * _libmusic_DSPCloseCommandsFile = (void *)0x04092d5c;
void * _libmusic_DSPCloseSimulatorFile = (void *)0x040920ea;
void * _libmusic_DSPDoubleToFix24 = (void *)0x04092144;
void * _libmusic_DSPDoubleToFix48UseArg = (void *)0x04092156;
void * _libmusic_DSPErrorNo = (void *)0x04080000;
void * _libmusic_DSPEnableErrorFile = (void *)0x4092174;
void * _libmusic_DSPFix24ToDouble = (void *)0x04092198;
void * _libmusic_DSPFix48ToDouble = (void *)0x040921c2;
void * _libmusic_DSPGetDSPCount = (void *)0x0409223a;
void * _libmusic_DSPGetHighestExternalUserAddress = (void *)0x04092ea0; 
void * _libmusic_DSPGetLowestExternalUserAddress = (void *)0x04092ea6; 
void * _libmusic_DSPGetSimulatorFP = (void *)0x040922f4;
void * _libmusic_DSPHostMessage = (void *)0x04092348;
void * _libmusic_DSPIntToFix24 = (void *)0x04092366;
void * _libmusic_DSPIsSavingCommands = (void *)0x04092dda;
void * _libmusic_DSPLCtoMS = (void *)0x0408002c;
void * _libmusic_DSPMKBLTTimed = (void *)0x04092b7c;
void * _libmusic_DSPMKCallTimedV = (void *)0x040923f6;
void * _libmusic_DSPMKClearDSPSoundOutBufferTimed = (void *)0x04092edc; 
void * _libmusic_DSPMKDisableAtomicTimed = (void *)0x04092408;
void * _libmusic_DSPMKDisableBlockingOnTMQEmptyTimed = (void *)0x0409240e;
void * _libmusic_DSPMKEnableAtomicTimed = (void *)0x0409242c;
void * _libmusic_DSPMKEnableBlockingOnTMQEmptyTimed = (void *)0x04092432;
void * _libmusic_DSPMKEnableReadData = (void *)0x04092d62;
void * _libmusic_DSPMKEnableSSISoundOut = (void *)0x04092a62;
void * _libmusic_DSPMKEnableSmallBuffers = (void *)0x04092438;
void * _libmusic_DSPMKEnableSoundOut = (void *)0x0409243e;
void * _libmusic_DSPMKEnableWriteData = (void *)0x04092444;
void * _libmusic_DSPMKFlushTimedMessages = (void *)0x04092450;
void * _libmusic_DSPMKGetClipCountAddress = (void *)0x04092ed0;
void * _libmusic_DSPMKFreezeOrchestra = (void *)0x04092d44;
void * _libmusic_DSPMKInit = (void *)0x040924a4;
void * _libmusic_DSPMKMemoryFillSkipTimed = (void *)0x04092bb2;
void * _libmusic_DSPMKPauseReadDataTimed = (void *)0x04092d86;
void * _libmusic_DSPMKPauseOrchestra = (void *)0x40924c2;
void * _libmusic_DSPMKReadTime = (void *)0x040924ce;
void * _libmusic_DSPMKResumeOrchestra = (void *)0x040924d4;
void * _libmusic_DSPMKResumeReadDataTimed = (void *)0x04092d8c;
void * _libmusic_DSPMKRetValueTimed = (void *)0x040924e0;
void * _libmusic_DSPMKRewindReadData = (void *)0x04092db6;
void * _libmusic_DSPMKRewindWriteData = (void *)0x040924e6;
void * _libmusic_DSPMKSendArraySkipTimed = (void *)0x04092e28;
void * _libmusic_DSPMKSendLongTimed = (void *)0x04092e34;
void * _libmusic_DSPMKSendShortArraySkipTimed = (void *)0x04092d3e;
void * _libmusic_DSPMKSendValueTimed = (void *)0x04092e2e;
void * _libmusic_DSPMKSetReadDataFile = (void *)0x04092d92;
void * _libmusic_DSPMKSetTime = (void *)0x04092504; 
void * _libmusic_DSPMKSetSamplingRate = (void *)0x040924fe;
void * _libmusic_DSPMKSetWriteDataFile = (void *)0x0409251c;
void * _libmusic_DSPMKStartReadDataTimed = (void *)0x04092d9e;
void * _libmusic_DSPMKStartSSISoundOut = (void *)0x04092a74;
void * _libmusic_DSPMKStartSoundOut = (void *)0x04092534;
void * _libmusic_DSPMKStartWriteDataTimed = (void *)0x04092540;
void * _libmusic_DSPMKStopReadData = (void *)0x04092db0;
void * _libmusic_DSPMKStopSSISoundOut = (void *)0x04092a80;
void * _libmusic_DSPMKStopSoundOut = (void *)0x04092546;
void * _libmusic_DSPMKStopWriteData = (void *)0x0409254c;
void * _libmusic_DSPMKThawOrchestra = (void *)0x04092d4a;
void * _libmusic_DSPMemoryNames = (void *)0x04094040;
void * _libmusic_DSPOpenCommandsFile = (void *)0x04092d56;
void * _libmusic_DSPOpenSimulatorFile = (void *)0x040925c4;
void * _libmusic_DSPSetCurrentDSP = (void *)0x04092708;
void * _libmusic_DSPStartAtAddress = (void *)0x04092762;
void * _libmusic_DSPWriteValue = (void *)0x04092e3a;
void * _libmusic__DSPError = (void *)0x04092912;
void * _libmusic__DSPMKSendUnitGeneratorWithLooperTimed = (void *)0x04092a6e;
void * _libmusic__DSPReloc = (void *)0x04092b28;

/* libNeXT */
void * _libmusic_DPSAddPort = (void *)0x060025fa;
void * _libmusic_DPSAddTimedEntry = (void *)0x06002600;
void * _libmusic_DPSRemovePort = (void *)0x0600266c;
void * _libmusic_DPSRemoveTimedEntry = (void *)0x06002672;
void * _libmusic_NXApp = (void *)0x04030000;
void * _libmusic_NXLogError = (void *)0x060023ba;

/* libsys */
void * _libmusic_NXClose = (void *)0x05003c74;
void * _libmusic_NXCountHashTable = (void *)0x05003938;
void * _libmusic_NXCreateHashTable = (void *)0x0500393e;
void * _libmusic_NXEmptyHashTable = (void *)0x05003944;
void * _libmusic_NXFlush = (void *)0x05003c98;
void * _libmusic_NXFreeHashTable = (void *)0x05003956;
void * _libmusic_NXHashGet = (void *)0x05003962;
void * _libmusic_NXHashInsert = (void *)0x05003968;
void * _libmusic_NXHashInsertIfAbsent = (void *)0x0500396e;
void * _libmusic_NXHashRemove = (void *)0x0500397a;
void * _libmusic_NXInitHashState = (void *)0x05003980;
void * _libmusic_NXNextHashState = (void *)0x05003986;
void * _libmusic_NXNoEffectFree = (void *)0x0500398c;
void * _libmusic_NXOpenFile = (void *)0x05003cb0;
void * _libmusic_NXPrintf = (void *)0x05003cc2;
void * _libmusic_NXReadArray = (void *)0x050039aa;
void * _libmusic_NXReadObject = (void *)0x050039b0;
void * _libmusic_NXReadType = (void *)0x050039bc;
void * _libmusic_NXReadTypes = (void *)0x050039c2;
void * _libmusic_NXSeek = (void *)0x05003cd4;
void * _libmusic_NXTell = (void *)0x05003cec;
void * _libmusic_NXTypedStreamClassVersion = (void *)0x050039e0;
void * _libmusic_NXUngetc = (void *)0x05003cf2;
void * _libmusic_NXUniqueString = (void *)0x05003a10;
void * _libmusic_NXWriteArray = (void *)0x05003a22;
void * _libmusic_NXWriteObject = (void *)0x05003a28;
void * _libmusic_NXWriteObjectReference = (void *)0x05003a2e;
void * _libmusic_NXWriteType = (void *)0x05003a40;
void * _libmusic_NXWriteTypes = (void *)0x05003a46;
void * _libmusic__NXStreamChangeBuffer = (void *)0x05003d22;
void * _libmusic__NXStreamFillBuffer = (void *)0x05003d28;
void * _libmusic__ctype_ = (void *)0x050062b0;
void * _libmusic__cvtToId = (void *)0x04010bd0;
void * _libmusic__fixunsdfsi = (void *)0x0500421a;
void * _libmusic__iob = (void *)0x04010000;
void * _libmusic__strhash = (void *)0x05003812;
void * _libmusic_alloca = (void *)0x0500217a;
void * _libmusic_atan2 = (void *)0x050036f2;
void * _libmusic_atoi = (void *)0x0500219e;
void * _libmusic_bsearch = (void *)0x050021fe;
void * _libmusic_calloc = (void *)0x0500220a;
void * _libmusic_close = (void *)0x0500229a;
void * _libmusic_cos = (void *)0x05003716;
void * _libmusic_creat = (void *)0x050022e8;
void * _libmusic_errno = (void *)0x040105b0;
void * _libmusic_exit = (void *)0x050024b0;
void * _libmusic_exp = (void *)0x05003734;
void * _libmusic_fabs = (void *)0x050024c2;
void * _libmusic_floor = (void *)0x05003746;
void * _libmusic_fprintf = (void *)0x0500252e;
void * _libmusic_free = (void *)0x05002546;
void * _libmusic_gettimeofday = (void *)0x05002738;
void * _libmusic_initstate = (void *)0x050027bc;
void * _libmusic_kern_timestamp = (void *)0x05002828;
void * _libmusic_log = (void *)0x05003770;
void * _libmusic_log10 = (void *)0x05003776;
void * _libmusic_longjmp = (void *)0x05002864;
void * _libmusic_mach_error_string = (void *)0x050028ca;
void * _libmusic_malloc = (void *)0x050028fa;
void * _libmusic_memcpy = (void *)0x05002948;
void * _libmusic_memmove = (void *)0x0500294e;
void * _libmusic_memset = (void *)0x05002954;
void * _libmusic_modf = (void *)0x0500299c;
void * _libmusic_msg_receive = (void *)0x050029ae;
void * _libmusic_name_server_port = (void *)0x04010294;
void * _libmusic_netname_look_up = (void *)0x05002a14;
void * _libmusic_objc_getClass = (void *)0x05003866;
void * _libmusic_objc_getMetaClass = (void *)0x05003872;
void * _libmusic_objc_msgSend = (void *)0x0500387e;
void * _libmusic_objc_msgSendSuper = (void *)0x05003884;
void * _libmusic_open = (void *)0x05002bc4;
void * _libmusic_port_allocate = (void *)0x05002c54;
void * _libmusic_port_deallocate = (void *)0x05002c5a;
void * _libmusic_pow = (void *)0x05003788;
void * _libmusic_qsort = (void *)0x05002d1a;
void * _libmusic_random = (void *)0x05002d38;
void * _libmusic_realloc = (void *)0x05002d7a;
void * _libmusic_setjmp = (void *)0x05002ec4;
void * _libmusic_setstate = (void *)0x05002f48;
void * _libmusic_sin = (void *)0x0500379a;
void * _libmusic_sprintf = (void *)0x05002fcc;
void * _libmusic_sqrt = (void *)0x050037a6;
void * _libmusic_sscanf = (void *)0x05002fde;
void * _libmusic_strcat = (void *)0x05002ff6;
void * _libmusic_strchr = (void *)0x05003002;
void * _libmusic_strcmp = (void *)0x05003008;
void * _libmusic_strcpy = (void *)0x0500301a;
void * _libmusic_strcspn = (void *)0x05003026;
void * _libmusic_strlen = (void *)0x05003038;
void * _libmusic_strncpy = (void *)0x0500304a;
void * _libmusic_strrchr = (void *)0x05003056;
void * _libmusic_strtod = (void *)0x05003068;
void * _libmusic_strtol = (void *)0x05003074;
void * _libmusic_task_self_ = (void *)0x04010290;
void * _libmusic_vfprintf = (void *)0x05003290;
void * _libmusic_vsprintf = (void *)0x050032ea;

void * _libmusic_NXVPrintf = (void *)0x05003cf8;
void * _libmusic_cthread_fork = (void *)0x0500230c;
void * _libmusic_cthread_set_errno_self = (void *)0x05003d46;
void * _libmusic_cthread_yield = (void *)0x05002342;
void * _libmusic_isinf = (void *)0x050027f8;
void * _libmusic_isnan = (void *)0x05002804;
void * _libmusic_msg_send = (void *)0x050029d2;
void * _libmusic_mutex_try_lock = (void *)0x050029ea;
void * _libmusic_mutex_wait_lock = (void *)0x050029f6;
void * _libmusic_port_set_add = (void *)0x05002c90;
void * _libmusic_port_set_remove = (void *)0x05002cae;
void * _libmusic_port_set_allocate = (void *)0x05002c96;
void * _libmusic_thread_info = (void *)0x050031a0;
void * _libmusic_thread_self = (void *)0x050031b2;
void * _libmusic_thread_priority = (void *)0x050040e8;
void * _libmusic_ur_cthread_self = (void *)0x0500325a;

void * _libmusic__fixdfsi = (void *)0x0500426e;
void * _libmusic_bcopy = (void *)0x050021c8;

void * _libmusic_thread_policy = (void *)0x05004100;
void * _libmusic_processor_set_default = (void *)0x0500409a;
void * _libmusic_host_processor_set_priv = (void *)0x05004076;
void * _libmusic_processor_set_policy_enable = (void *)0x05004106;
void * _libmusic_processor_set_policy_disable = (void *)0x0500410c;
void * _libmusic_geteuid = (void *)0x050025d6;
void * _libmusic_host_self = (void *)0x05004010;
void * _libmusic_host_priv_self = (void *)0x05004016;

void * _libmusic_NXDefaultMallocZone = (void *)0x05002444;
void * _libmusic_cthread_abort = (void *)0x050037d6;
void * _libmusic_condition_wait = (void *)0x050022be;
void * _libmusic_cond_signal = (void *)0x050022b8;
void * _libmusic_objc_getClassWithoutWarning = (void *)0x050038c6;
void * _libmusic_cthread_detach = (void *)0x05002300;

/*
 * New pointers are added before this symbol.  This must remain at the end of
 * this file.  When a new pointers are added the pad must be reduced by the
 * number of new pointers added.
 */
static void *_libmusic_pointers_pad[294] = { 0 };


