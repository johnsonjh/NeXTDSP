#ifdef SHLIB
/*
 * This file is only used when building a shared version of libdsp.
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
 */

/* In libsys_s.a */
#define	free(a) (*_libdsp_free)(a)
#define	open(a,b,c) (*_libdsp_open)(a,b,c)
#define select(a,b,c,d,e) (*_libdsp_select)(a,b,c,d,e)
#define	strncpy(a,b,c) (*_libdsp_strncpy)(a,b,c)
#define	time(a) (*_libdsp_time)(a)

#define SNDAlloc (*_libdsp_SNDAlloc)
#define SNDFree (*_libdsp_SNDFree)
#define _ctype_ (*_libdsp__ctype_)
#define _filbuf (*_libdsp__filbuf)
#define _flsbuf (*_libdsp__flsbuf)
#define _iob (*_libdsp__iob)
#define access (*_libdsp_access)
#define alloca (*_libdsp_alloca)
#define atoi (*_libdsp_atoi)
#define cthread_fork (*_libdsp_cthread_fork)
#define cthread_init (*_libdsp_cthread_init)
#define cthread_join (*_libdsp_cthread_join)
#define cthread_yield (*_libdsp_cthread_yield)
#define ctime (*_libdsp_ctime)
#define errno (*_libdsp_errno)
#define exit (*_libdsp_exit)
#define fclose (*_libdsp_fclose)
#define fflush (*_libdsp_fflush)
#define fgets (*_libdsp_fgets)
#define fopen (*_libdsp_fopen)
#define fprintf (*_libdsp_fprintf)
#define fread (*_libdsp_fread)
#define fwrite (*_libdsp_fwrite)
#define getenv (*_libdsp_getenv)
#define getpagesize (*_libdsp_getpagesize)
#define getpid (*_libdsp_getpid)
#define gettimeofday (*_libdsp_gettimeofday)
#define isatty (*_libdsp_isatty)
#define mach_error_string (*_libdsp_mach_error_string)
#define malloc (*_libdsp_malloc)
#define malloc_size (*_libdsp_malloc_size)
#define memmove (*_libdsp_memmove)
#define mmap (*_libdsp_mmap)
#define msg_receive (*_libdsp_msg_receive)
#define msg_rpc (*_libdsp_msg_rpc)
#define msg_send (*_libdsp_msg_send)
#define name_server_port (*_libdsp_name_server_port)
#define netname_look_up (*_libdsp_netname_look_up)
#define pause (*_libdsp_pause)
#define perror (*_libdsp_perror)
#define port_allocate (*_libdsp_port_allocate)
#define port_deallocate (*_libdsp_port_deallocate)
#define printf (*_libdsp_printf)
#define realloc (*_libdsp_realloc)
#define rewind (*_libdsp_rewind)
#define setlinebuf (*_libdsp_setlinebuf)
#define snddriver_dspcmd_req_condition (*_libdsp_snddriver_dspcmd_req_condition)
#define sprintf (*_libdsp_sprintf)
#define sscanf (*_libdsp_sscanf)
#define strchr (*_libdsp_strchr)
#define strcmp (*_libdsp_strcmp)
#define strcpy (*_libdsp_strcpy)
#define strlen (*_libdsp_strlen)
#define strncmp (*_libdsp_strncmp)
#define strtol (*_libdsp_strtol)
#define sys_errlist (*_libdsp_sys_errlist)
#define sys_nerr (*_libdsp_sys_nerr)
#define task_self_ (*_libdsp_task_self_)
#define thread_reply (*_libdsp_thread_reply)
#define umask (*_libdsp_umask)
#define ungetc (*_libdsp_ungetc)
#define unlink (*_libdsp_unlink)
#define usleep (*_libdsp_usleep)
#define valloc (*_libdsp_valloc)
#define vm_deallocate (*_libdsp_vm_deallocate)

/* 04/10/90/jos for 2.0 */
#define vm_page_size (*_libdsp_vm_page_size)
#define close (*_libdsp_close)
#define read (*_libdsp_read)
#define SNDSoundError (*_libdsp_SNDSoundError)
#define SNDReadHeader (*_libdsp_SNDReadHeader)
#define snddriver_stream_setup (*_libdsp_snddriver_stream_setup)
#define snddriver_stream_start_reading (*_libdsp_snddriver_stream_start_reading)
#define snddriver_stream_start_writing (*_libdsp_snddriver_stream_start_writing)
#define snddriver_stream_control (*_libdsp_snddriver_stream_control)
#define snddriver_stream_nsamples (*_libdsp_snddriver_stream_nsamples)
#define snddriver_get_dsp_cmd_port (*_libdsp_snddriver_get_dsp_cmd_port)
#define snddriver_set_dsp_owner_port (*_libdsp_snddriver_set_dsp_owner_port)
#define snddriver_set_sndout_owner_port (*_libdsp_snddriver_set_sndout_owner_port)
#define snddriver_dsp_protocol (*_libdsp_snddriver_dsp_protocol)
#define snddriver_dsp_reset (*_libdsp_snddriver_dsp_reset)
#define snddriver_dspcmd_req_err (*_libdsp_snddriver_dspcmd_req_err)
#define snddriver_dspcmd_req_msg (*_libdsp_snddriver_dspcmd_req_msg)
#define snddriver_reply_handler (*_libdsp_snddriver_reply_handler)
#define lseek (*_libdsp_lseek)
#define fixdfsi (*_libdsp__fixdfsi)
#define write (*_libdsp_write)
#define bootstrap_look_up (*_libdsp_bootstrap_look_up)
#define SNDAcquire (*_libdsp_SNDAcquire)
#define SNDRelease (*_libdsp_SNDRelease)
#define snddriver_set_sndout_bufsize (*_libdsp_snddriver_set_sndout_bufsize)
#define snddriver_set_sndout_bufcount (*_libdsp_snddriver_set_sndout_bufcount)

#endif SHLIB
