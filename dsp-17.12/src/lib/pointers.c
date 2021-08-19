/*
 * This file contains the pointers to symbols not defined in libdsp but used
 * by the code in libdsp.  For the pointers who's symbols are expected to be
 * in libsys they are initialized to the addresses in libsys (for routines this
 * is the address of a branch table slot not the actual address of the routine).
 *
 * New pointers are to be added at the end of this file before the symbol 
 * _libdsp_pointers_pad and the pad to be reduced by the number of pointers
 * added.  This keeps the global data size of this file constant and thus does
 * not effect the addresses of later symbols.
 *
 * (As of 4/17/90, there are no later symbols, so padding doesn't matter. JOS)
 *
 * Modification history
 * 04/10/90/jos - added many pointers for 2.0 shlib (see list at end)
 * 04/16/90/jos - added `cat new_libsys_ptrs` and started padding at 100
 * 10/01/90/jos - retired "snddriver_dsp_buffers_per_soundout_buffer" and added
 *                "snddriver_set_sndout_bufsize" and
 *                "snddriver_set_sndout_bufcount"

ADDING NEW POINTERS

Kevin Enderby has a program for finding the entry points.  First create a
file containing the symbol names (as(1) nm prints them), for example a file
named "list" contains:

_vm_page_size
_close

And then run the following program using the above "list" file and it will
print on stdout:

~enderby/bin/pointers _libdsp list /lib/libsys_s.a
void * _libdsp_vm_page_size = (void *)0x040102a0;
void * _libdsp_close = (void *)0x0500229a;

Don't for get to adjust your pointers pad, add #init's to your spec_dsp
file and macros to your shlib.h file.

*/

/* Defined in libsys_s.a */
void * _libdsp_SNDAlloc = (void *)0x05003ab8;
void * _libdsp_SNDFree = (void *)0x05003ae8;
void * _libdsp__ctype_ = (void *)0x050062b0;
void * _libdsp__filbuf = (void *)0x0500208a;
void * _libdsp__flsbuf = (void *)0x050020a2;
void * _libdsp__iob = (void *)0x04010000;
void * _libdsp_access = (void *)0x05002138;
void * _libdsp_alloca = (void *)0x0500217a;
void * _libdsp_atoi = (void *)0x0500219e;
void * _libdsp_bcopy = (void *)0x050021c8;
void * _libdsp_cthread_fork = (void *)0x0500230c;
void * _libdsp_cthread_init = (void *)0x05002312;
void * _libdsp_cthread_join = (void *)0x05002318;
void * _libdsp_cthread_yield = (void *)0x05002342;
void * _libdsp_ctime = (void *)0x05002348;
void * _libdsp_errno = (void *)0x040105b0;
void * _libdsp_exit = (void *)0x050024b0;
void * _libdsp_fclose = (void *)0x050024d4;
void * _libdsp_fflush = (void *)0x050024f8;
void * _libdsp_fgets = (void *)0x05002510;
void * _libdsp_fopen = (void *)0x0500251c;
void * _libdsp_fprintf = (void *)0x0500252e;
void * _libdsp_fread = (void *)0x05002540;
void * _libdsp_free = (void *)0x05002546;
void * _libdsp_fwrite = (void *)0x0500258e;
void * _libdsp_getenv = (void *)0x050025d0;
void * _libdsp_getpagesize = (void *)0x05002678;
void * _libdsp_getpid = (void *)0x05002690;
void * _libdsp_gettimeofday = (void *)0x05002738;
void * _libdsp_isatty = (void *)0x050027e0;
void * _libdsp_mach_error_string = (void *)0x050028ca;
void * _libdsp_malloc = (void *)0x050028fa;
void * _libdsp_malloc_size = (void *)0x05002912;
void * _libdsp_memmove = (void *)0x0500294e;
void * _libdsp_mmap = (void *)0x05002996;
void * _libdsp_msg_receive = (void *)0x050029ae;
void * _libdsp_msg_rpc = (void *)0x050029c0;
void * _libdsp_msg_send = (void *)0x050029d2;
void * _libdsp_name_server_port = (void *)0x04010294;
void * _libdsp_netname_look_up = (void *)0x05002a14;
void * _libdsp_open = (void *)0x05002bc4;
void * _libdsp_pause = (void *)0x05002c18;
void * _libdsp_perror = (void *)0x05002c24;
void * _libdsp_port_allocate = (void *)0x05002c54;
void * _libdsp_port_deallocate = (void *)0x05002c5a;
void * _libdsp_printf = (void *)0x05002ce4;
void * _libdsp_realloc = (void *)0x05002d7a;
void * _libdsp_rewind = (void *)0x05002dec;
void * _libdsp_select = (void *)0x05002e3a;
void * _libdsp_setlinebuf = (void *)0x05002ed0;
void * _libdsp_snddriver_dspcmd_req_condition = (void *)0x05003bcc;
void * _libdsp_sprintf = (void *)0x05002fcc;
void * _libdsp_sscanf = (void *)0x05002fde;
void * _libdsp_strchr = (void *)0x05003002;
void * _libdsp_strcmp = (void *)0x05003008;
void * _libdsp_strcpy = (void *)0x0500301a;
void * _libdsp_strlen = (void *)0x05003038;
void * _libdsp_strncmp = (void *)0x05003044;
void * _libdsp_strncpy = (void *)0x0500304a;
void * _libdsp_strtol = (void *)0x05003074;
void * _libdsp_sys_errlist = (void *)0x05006004;
void * _libdsp_sys_nerr = (void *)0x050061e4;
void * _libdsp_task_self_ = (void *)0x04010290;
void * _libdsp_thread_reply = (void *)0x050031a6;
void * _libdsp_time = (void *)0x050031d0;
void * _libdsp_umask = (void *)0x0500323c;
void * _libdsp_ungetc = (void *)0x05003242;
void * _libdsp_unlink = (void *)0x0500324e;
void * _libdsp_usleep = (void *)0x05003260;
void * _libdsp_valloc = (void *)0x05003284;
void * _libdsp_vm_deallocate = (void *)0x050032ba;

/* Added for 2.0. 04/10/90/jos */
void * _libdsp_vm_page_size = (void *)0x040102a0;
void * _libdsp_close = (void *)0x0500229a;
void * _libdsp_read = (void *)0x05002d62;
void * _libdsp_SNDSoundError = (void *)0x05003b6c;
void * _libdsp_SNDReadHeader = (void *)0x05003b24;
void * _libdsp_snddriver_stream_setup = (void *)0x05003c20;
void * _libdsp_snddriver_stream_start_reading = (void *)0x05003c26;
void * _libdsp_snddriver_stream_start_writing = (void *)0x05003c2c;
void * _libdsp_snddriver_stream_control = (void *)0x05003c14;
void * _libdsp_snddriver_stream_nsamples = (void *)0x05003c1a;
void * _libdsp_snddriver_get_dsp_cmd_port = (void *)0x05003be4;
void * _libdsp_snddriver_set_dsp_buffers_per_soundout_buffer=(void*)0x0;
void * _libdsp_snddriver_set_dsp_owner_port = (void *)0x05003bfc;
void * _libdsp_snddriver_set_sndout_owner_port = (void *)0x05003c08;
void * _libdsp_snddriver_dsp_protocol = (void *)0x05003bb4;
void * _libdsp_snddriver_dsp_reset = (void *)0x05003c5c;
void * _libdsp_snddriver_dspcmd_req_err = (void *)0x05003bd2;
void * _libdsp_snddriver_dspcmd_req_msg = (void *)0x05003bd8;
void * _libdsp_snddriver_reply_handler = (void *)0x05003bf0;
void * _libdsp_lseek = (void *)0x050028ac;
void * _libdsp__fixunsdfsi = (void *)0x0500421a;
void * _libdsp__fixdfsi = (void *)0x0500426e;
void * _libdsp_bootstrap_look_up = (void *)0x0500416c;
void * _libdsp_SNDAcquire = (void *)0x05003ab2;
void * _libdsp_SNDRelease = (void *)0x05003b30;
void * _libdsp_snddriver_set_sndout_bufsize = (void *)0x050042ec;
void * _libdsp_snddriver_set_sndout_bufcount = (void *)0x050042f2;

/*
 * New pointers are added before this symbol.  This must remain at the end of
 * this file.  When a new pointers are added the pad must be reduced by the
 * number of new pointers added.  This symbol must be global and NOT static
 * because it is padding the size of global data in this file so that the size
 * is a constant.  If it were to be static ld(1) would not allow it to be
 * loaded into a shlib because the data would be out of order (static data
 * following global data) and since ld(1) splits global and static data when
 * loaded into a shlib this wouldn't pad the global data.
 */
void *_libdsp_pointers_pad[95] = { 0 };
