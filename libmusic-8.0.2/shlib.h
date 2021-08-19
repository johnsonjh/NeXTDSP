#ifdef SHLIB
/*
 * This file is only used when building a shared version of libmusic.
 * It's purpose is to turn references to symbols not defined in the
 * library into indirect references, via a pointer that will be defined
 * in the library (see the file pointers.c).  These macros are expected
 * to be substituted for both the references and the declarations.  In
 * the case the user supplies the declaration (in his code or and include
 * file) the type for the pointer is declared when the macro is substitued.
 * The user MUST supply a declaration for each of these symbols he uses in
 * his code.
 *
 * MAJOR ASSUMPTION here is that the object files of this library are not
 * expected to be substituted by the user.  This means that the inter module
 * references are linked directly to each other and if the user attempted to
 * replace an object file he would get a multiply defined error.

 * PLEASE MAINTAIN FUNCTIONS IN ALPHABETICAL ORDER - DAJ.
 */

/* Modification history:
        05/05/90 - Daj added DSPMKSetTime() and 
                   DSPMKClearDSPSoundOutBufferTimed(). Removed DSPMKWriteLong()
                   and DSPMemoryClear()
        05/06/90 - Daj added DSPGetLowestExternalUserAddress(),
                   DSPGetHighestExternalUserAddress(), 
                   DSPMKGetClipCountAddress(),
        05/13/90 - JOS added 7 new math.h functions. 
	           (sin,cos,exp,sqrt,log,log10,fabs)
	           I reordered them.
        6/10/90 -  daj added DSPMKPauseOrchestra.
        7/25/90 -  daj added the following libsys symbols that are needed
		   by _MKSprintf:  isnan, isinf, _dbltopdfp
		   Also added libsys symbols: cthread_fork, cthread_yield,
		   thread_info, thread_priority, cthread_set_errno_self
	07/26/90 - Added libsys symbols:
                   mutex_try_lock, mutex_wait_lock, msg_send, port_set_add,
		   port_set_remove, port_set_allocate, thread_self,
                   ur_cthread_self, NXVPrintf
	08/03/90 - Removed _dbltopdfp
        08/13/90 - Added libsys functions: thread_policy, 
                   processor_set_default, host_processor_set_priv,
                   processor_set_policy_enable
                   processor_set_policy_disable
                   geteuid, host_self, host_priv_self
        08/13/90 - Added NXDefaultMallocZone().
        09/29/90 - Added cthread_abort(), condition_signal(), condition_wait(),
	           condition_alloc(), _objc_getClassWithoutWarning,
		   cthread_detach()
*/

/* libmidi */
#define midi_clear_queue (*_libmusic_midi_clear_queue)
#define midi_error_string (*_libmusic_midi_error_string)
#define midi_get_data (*_libmusic_midi_get_data)
#define midi_get_in_timer_port (*_libmusic_midi_get_in_timer_port)
#define midi_get_recv (*_libmusic_midi_get_recv)
#define midi_get_xmit (*_libmusic_midi_get_xmit)
#define midi_output_queue_notify (*_libmusic_midi_output_queue_notify)
#define midi_reply_handler (*_libmusic_midi_reply_handler)
#define midi_send_raw_data (*_libmusic_midi_send_raw_data)
#define midi_set_owner (*_libmusic_midi_set_owner)
#define midi_set_proto (*_libmusic_midi_set_proto)
#define midi_set_sys_ignores (*_libmusic_midi_set_sys_ignores)
#define midi_timer_error_string (*_libmusic_midi_timer_error_string)

/* libdsp */
#define DSPClose (*_libmusic_DSPClose)
#define DSPCloseCommandsFile (*_libmusic_DSPCloseCommandsFile)
#define DSPCloseSimulatorFile (*_libmusic_DSPCloseSimulatorFile)
#define DSPDoubleToFix24 (*_libmusic_DSPDoubleToFix24)
#define DSPDoubleToFix48UseArg (*_libmusic_DSPDoubleToFix48UseArg)
#define DSPEnableErrorFile (*_libmusic_DSPEnableErrorFile)
#define DSPErrorNo (*_libmusic_DSPErrorNo)
#define DSPFix24ToDouble (*_libmusic_DSPFix24ToDouble)
#define DSPFix48ToDouble (*_libmusic_DSPFix48ToDouble)
#define DSPGetDSPCount (*_libmusic_DSPGetDSPCount)
#define DSPGetSimulatorFP (*_libmusic_DSPGetSimulatorFP)
#define DSPGetHighestExternalUserAddress (*_libmusic_DSPGetHighestExternalUserAddress)
#define DSPGetLowestExternalUserAddress (*_libmusic_DSPGetLowestExternalUserAddress)
#define DSPHostMessage (*_libmusic_DSPHostMessage)
#define DSPIntToFix24 (*_libmusic_DSPIntToFix24)
#define DSPIsSavingCommands (*_libmusic_DSPIsSavingCommands)
#define DSPLCtoMS (*_libmusic_DSPLCtoMS)
#define DSPMKBLTTimed (*_libmusic_DSPMKBLTTimed)
#define DSPMKCallTimedV (*_libmusic_DSPMKCallTimedV)
#define DSPMKClearDSPSoundOutBufferTimed (*_libmusic_DSPMKClearDSPSoundOutBufferTimed)
#define DSPMKDisableAtomicTimed (*_libmusic_DSPMKDisableAtomicTimed)
#define DSPMKDisableBlockingOnTMQEmptyTimed (*_libmusic_DSPMKDisableBlockingOnTMQEmptyTimed)
#define DSPMKEnableAtomicTimed (*_libmusic_DSPMKEnableAtomicTimed)
#define DSPMKEnableBlockingOnTMQEmptyTimed (*_libmusic_DSPMKEnableBlockingOnTMQEmptyTimed)
#define DSPMKEnableReadData (*_libmusic_DSPMKEnableReadData)
#define DSPMKEnableSSISoundOut (*_libmusic_DSPMKEnableSSISoundOut)
#define DSPMKEnableSmallBuffers (*_libmusic_DSPMKEnableSmallBuffers)
#define DSPMKEnableSoundOut (*_libmusic_DSPMKEnableSoundOut)
#define DSPMKEnableWriteData (*_libmusic_DSPMKEnableWriteData)
#define DSPMKFlushTimedMessages (*_libmusic_DSPMKFlushTimedMessages)
#define DSPMKFreezeOrchestra (*_libmusic_DSPMKFreezeOrchestra)
#define DSPMKGetClipCountAddress (*_libmusic_DSPMKGetClipCountAddress)
#define DSPMKInit (*_libmusic_DSPMKInit)
#define DSPMKMemoryFillSkipTimed (*_libmusic_DSPMKMemoryFillSkipTimed)
#define DSPMKPauseOrchestra (*_libmusic_DSPMKPauseOrchestra)
#define DSPMKPauseReadDataTimed (*_libmusic_DSPMKPauseReadDataTimed)
#define DSPMKReadTime (*_libmusic_DSPMKReadTime)
#define DSPMKResumeOrchestra (*_libmusic_DSPMKResumeOrchestra)
#define DSPMKResumeReadDataTimed (*_libmusic_DSPMKResumeReadDataTimed)
#define DSPMKRetValueTimed (*_libmusic_DSPMKRetValueTimed)
#define DSPMKRewindReadData (*_libmusic_DSPMKRewindReadData)
#define DSPMKRewindWriteData (*_libmusic_DSPMKRewindWriteData)
#define DSPMKSendArraySkipTimed (*_libmusic_DSPMKSendArraySkipTimed)
#define DSPMKSendLongTimed (*_libmusic_DSPMKSendLongTimed)
#define DSPMKSendShortArraySkipTimed (*_libmusic_DSPMKSendShortArraySkipTimed)
#define DSPMKSendValueTimed (*_libmusic_DSPMKSendValueTimed)
#define DSPMKSetReadDataFile (*_libmusic_DSPMKSetReadDataFile)
#define DSPMKSetSamplingRate (*_libmusic_DSPMKSetSamplingRate)
#define DSPMKSetTime (*_libmusic_DSPMKSetTime)
#define DSPMKSetWriteDataFile (*_libmusic_DSPMKSetWriteDataFile)
#define DSPMKStartReadDataTimed (*_libmusic_DSPMKStartReadDataTimed)
#define DSPMKStartSSISoundOut (*_libmusic_DSPMKStartSSISoundOut)
#define DSPMKStartSoundOut (*_libmusic_DSPMKStartSoundOut)
#define DSPMKStartWriteDataTimed (*_libmusic_DSPMKStartWriteDataTimed)
#define DSPMKStopReadData (*_libmusic_DSPMKStopReadData)
#define DSPMKStopSSISoundOut (*_libmusic_DSPMKStopSSISoundOut)
#define DSPMKStopSoundOut (*_libmusic_DSPMKStopSoundOut)
#define DSPMKStopWriteData (*_libmusic_DSPMKStopWriteData)
#define DSPMKThawOrchestra (*_libmusic_DSPMKThawOrchestra)
#define DSPMemoryNames (*_libmusic_DSPMemoryNames)
#define DSPOpenCommandsFile (*_libmusic_DSPOpenCommandsFile)
#define DSPOpenSimulatorFile (*_libmusic_DSPOpenSimulatorFile)
#define DSPSetCurrentDSP (*_libmusic_DSPSetCurrentDSP)
#define DSPStartAtAddress (*_libmusic_DSPStartAtAddress)
#define DSPWriteValue (*_libmusic_DSPWriteValue)
#define _DSPError (*_libmusic__DSPError)
#define _DSPMKSendUnitGeneratorWithLooperTimed (*_libmusic__DSPMKSendUnitGeneratorWithLooperTimed)
#define _DSPReloc (*_libmusic__DSPReloc)

/* libNext */
#define NXApp (*_libmusic_NXApp)
#define NXLogError (*_libmusic_NXLogError)
#define DPSAddPort (*_libmusic_DPSAddPort)
#define DPSAddTimedEntry (*_libmusic_DPSAddTimedEntry)
#define DPSRemovePort (*_libmusic_DPSRemovePort)
#define DPSRemoveTimedEntry (*_libmusic_DPSRemoveTimedEntry)

/* libsys */
#define NXReadArray (*_libmusic_NXReadArray)
#define NXReadObject (*_libmusic_NXReadObject)
#define NXReadType (*_libmusic_NXReadType)
#define NXReadTypes (*_libmusic_NXReadTypes)
#define NXTypedStreamClassVersion (*_libmusic_NXTypedStreamClassVersion)
#define NXWriteArray (*_libmusic_NXWriteArray)
#define NXWriteObject (*_libmusic_NXWriteObject)
#define NXWriteObjectReference (*_libmusic_NXWriteObjectReference)
#define NXWriteType (*_libmusic_NXWriteType)
#define NXWriteTypes (*_libmusic_NXWriteTypes)
#define NXCountHashTable (*_libmusic_NXCountHashTable)
#define NXCreateHashTable (*_libmusic_NXCreateHashTable)
#define NXDefaultMallocZone (*_libmusic_NXDefaultMallocZone)
#define NXEmptyHashTable (*_libmusic_NXEmptyHashTable)
#define NXFlush (*_libmusic_NXFlush)
#define NXFreeHashTable (*_libmusic_NXFreeHashTable)
#define NXHashGet (*_libmusic_NXHashGet)
#define NXHashInsert (*_libmusic_NXHashInsert)
#define NXHashInsertIfAbsent (*_libmusic_NXHashInsertIfAbsent)
#define NXHashRemove (*_libmusic_NXHashRemove)
#define NXInitHashState (*_libmusic_NXInitHashState)
#define NXNextHashState (*_libmusic_NXNextHashState)
#define NXNoEffectFree (*_libmusic_NXNoEffectFree)
#define NXOpenFile (*_libmusic_NXOpenFile)
#define NXPrintf (*_libmusic_NXPrintf)
#define NXVPrintf (*_libmusic_NXVPrintf)
#define NXClose (*_libmusic_NXClose)
#define NXSeek (*_libmusic_NXSeek)
#define NXTell (*_libmusic_NXTell)
#define NXUngetc (*_libmusic_NXUngetc)
#define NXUniqueString (*_libmusic_NXUniqueString)
#define _NXStreamChangeBuffer (*_libmusic__NXStreamChangeBuffer)
#define _NXStreamFillBuffer (*_libmusic__NXStreamFillBuffer)
#define _ctype_ (*_libmusic__ctype_)
#define _cvtToId (*_libmusic__cvtToId)
#define _fixunsdfsi (*_libmusic__fixunsdfsi)
#define _iob (*_libmusic__iob)
#define _strhash (*_libmusic__strhash)
#define alloca (*_libmusic_alloca)
#define atan2 (*_libmusic_atan2)
#define atoi (*_libmusic_atoi)
#define bsearch (*_libmusic_bsearch)
#define calloc (*_libmusic_calloc)
#define close(a) (*_libmusic_close)(a)
#define cond_signal (*_libmusic_cond_signal)
#define condition_wait (*_libmusic_condition_wait)
#define cos (*_libmusic_cos)
#define creat (*_libmusic_creat)
#define cthread_abort (*_libmusic_cthread_abort)
#define cthread_detach (*_libmusic_cthread_detach)
#define cthread_fork (*_libmusic_cthread_fork)
#define cthread_set_errno_self (*_libmusic_cthread_set_errno_self)
#define cthread_yield (*_libmusic_cthread_yield)
#define errno (*_libmusic_errno)
#define exit (*_libmusic_exit)
#define exp (*_libmusic_exp)
#define fabs (*_libmusic_fabs)
#define floor(a) (*_libmusic_floor)(a)
#define fprintf (*_libmusic_fprintf)
#define free(a) (*_libmusic_free)(a)
#define gettimeofday (*_libmusic_gettimeofday)
#define geteuid (*_libmusic_geteuid)
#define host_processor_set_priv (*_libmusic_host_processor_set_priv)
#define host_self (*_libmusic_host_self)
#define host_priv_self (*_libmusic_host_priv_self)
#define initstate (*_libmusic_initstate)
#define isnan (*_libmusic_isnan)
#define isinf (*_libmusic_isinf)
#define kern_timestamp (*_libmusic_kern_timestamp)
#define log (*_libmusic_log)
#define log10 (*_libmusic_log10)
#define longjmp (*_libmusic_longjmp)
#define mach_error_string (*_libmusic_mach_error_string)
#define malloc (*_libmusic_malloc)
#define memcpy (*_libmusic_memcpy)
#define memmove (*_libmusic_memmove)
#define memset (*_libmusic_memset)
#define modf (*_libmusic_modf)
#define mutex_try_lock (*_libmusic_mutex_try_lock)
#define mutex_wait_lock (*_libmusic_mutex_wait_lock)
#define msg_receive (*_libmusic_msg_receive)
#define msg_send (*_libmusic_msg_send)
#define name_server_port (*_libmusic_name_server_port)
#define netname_look_up (*_libmusic_netname_look_up)
#define objc_getClass (*_libmusic_objc_getClass)
#define objc_getClassWithoutWarning (*_libmusic_objc_getClassWithoutWarning)
#define objc_getMetaClass (*_libmusic_objc_getMetaClass)
#define objc_msgSend (*_libmusic_objc_msgSend)
#define objc_msgSendSuper (*_libmusic_objc_msgSendSuper)
#define open(a,b,c) (*_libmusic_open)(a,b,c)
#define port_allocate (*_libmusic_port_allocate)
#define port_deallocate (*_libmusic_port_deallocate)
#define port_set_add (*_libmusic_port_set_add)
#define port_set_remove (*_libmusic_port_set_remove)
#define port_set_allocate (*_libmusic_port_set_allocate)
#define pow (*_libmusic_pow)
#define processor_set_default (*_libmusic_processor_set_default)
#define processor_set_policy_enable (*_libmusic_processor_set_policy_enable)
#define processor_set_policy_disable (*_libmusic_processor_set_policy_disable)
#define qsort (*_libmusic_qsort)
#define random (*_libmusic_random)
#define realloc (*_libmusic_realloc)
#define setjmp (*_libmusic_setjmp)
#define setstate (*_libmusic_setstate)
#define sin (*_libmusic_sin)
#define sprintf (*_libmusic_sprintf)
#define sqrt (*_libmusic_sqrt)
#define sscanf (*_libmusic_sscanf)
#define strcat(a,b) (*_libmusic_strcat)(a,b)
#define strchr (*_libmusic_strchr)
#define strcmp (*_libmusic_strcmp)
#define strcpy (*_libmusic_strcpy)
#define strcspn (*_libmusic_strcspn)
#define strlen (*_libmusic_strlen)
#define strncpy(a,b,c) (*_libmusic_strncpy)(a,b,c)
#define strrchr (*_libmusic_strrchr)
#define strtod (*_libmusic_strtod)
#define strtol (*_libmusic_strtol)
#define task_self_ (*_libmusic_task_self_)
#define thread_info (*_libmusic_thread_info)
#define thread_policy (*_libmusic_thread_policy)
#define thread_priority (*_libmusic_thread_priority)
#define thread_self (*_libmusic_thread_self)
#define thread_terminate (*_libmusic_thread_terminate)
#define timer_set (*_libmusic_timer_set)
#define timer_start (*_libmusic_timer_start)
#define timer_stop (*_libmusic_timer_stop)
#define ur_cthread_self (*_libmusic_ur_cthread_self)
#define vfprintf (*_libmusic_vfprintf)
#define vsprintf (*_libmusic_vsprintf)

#endif SHLIB


