/* 
    keynums.h 
    Copyright 1989, NeXT, Inc.
    
    This file is part of the Music Kit.
  */
#ifndef KEYNUMS_H
#define KEYNUMS_H

/* Here are the midi key macro definitions. 

   For the sake of terseness and convenience of use, we break here from the 
   Music Kit convention of appending MK_ to the start of macros. 
  */

typedef enum _MKKeyNum {
    c00k =0,cs00k,d00k,ef00k,e00k,f00k,fs00k,g00k,af00k,a00k,bf00k,b00k,
    c0k,cs0k,d0k,ef0k,e0k,f0k,fs0k,g0k,af0k,a0k,bf0k,b0k,
    c1k,cs1k,d1k,ef1k,e1k,f1k,fs1k,g1k,af1k,a1k,bf1k,b1k,
    c2k,cs2k,d2k,ef2k,e2k,f2k,fs2k,g2k,af2k,a2k,bf2k,b2k,
    c3k,cs3k,d3k,ef3k,e3k,f3k,fs3k,g3k,af3k,a3k,bf3k,b3k,
    c4k,cs4k,d4k,ef4k,e4k,f4k,fs4k,g4k,af4k,a4k,bf4k,b4k,
    c5k,cs5k,d5k,ef5k,e5k,f5k,fs5k,g5k,af5k,a5k,bf5k,b5k,
    c6k,cs6k,d6k,ef6k,e6k,f6k,fs6k,g6k,af6k,a6k,bf6k,b6k,
    c7k,cs7k,d7k,ef7k,e7k,f7k,fs7k,g7k,af7k,a7k,bf7k,b7k,
    c8k,cs8k,d8k,ef8k,e8k,f8k,fs8k,g8k,af8k,a8k,bf8k,b8k,
    c9k,cs9k,d9k,ef9k,e9k,f9k,fs9k,g9k} MKKeyNum;

/* Enharmonic equivalents for the above */

#define df00k cs00k
#define ds00k ef00k
#define es00k f00k 
#define ff00k e00k  
#define gf00k fs00k
#define gs00k af00k
#define as00k bf00k
#define cf0k b00k 
#define bs00k c0k

#define df0k cs0k
#define ds0k ef0k
#define es0k f0k 
#define ff0k e0k  
#define gf0k fs0k
#define gs0k af0k
#define as0k bf0k
#define bs0k  c1k 
#define cf1k b0k  

#define bs0k c1k
#define df1k cs1k
#define ds1k ef1k
#define es1k f1k 
#define ff1k e1k  
#define gf1k fs1k
#define gs1k af1k
#define as1k bf1k
#define bs1k  c2k 
#define cf2k b1k  

#define bs1k c2k
#define df2k cs2k
#define ds2k ef2k
#define es2k f2k 
#define ff2k e2k  
#define gf2k fs2k
#define gs2k af2k
#define as2k bf2k
#define bs2k  c3k 
#define cf3k b2k  

#define bs2k c3k
#define df3k cs3k
#define ds3k ef3k
#define es3k f3k 
#define ff3k e3k  
#define gf3k fs3k
#define gs3k af3k
#define as3k bf3k
#define bs3k  c4k 
#define cf4k b3k  

#define bs3k c4k
#define df4k cs4k
#define ds4k ef4k
#define es4k f4k 
#define ff4k e4k  
#define gf4k fs4k
#define gs4k af4k
#define as4k bf4k
#define bs4k  c5k 
#define cf5k b4k  

#define bs4k c5k
#define df5k cs5k
#define ds5k ef5k
#define es5k f5k 
#define ff5k e5k  
#define gf5k fs5k
#define gs5k af5k
#define as5k bf5k
#define bs5k  c6k 
#define cf6k b5k  

#define bs5k c6k
#define df6k cs6k
#define ds6k ef6k
#define es6k f6k 
#define ff6k e6k  
#define gf6k fs6k
#define gs6k af6k
#define as6k bf6k
#define bs6k  c7k 
#define cf7k b6k  

#define bs6k c7k
#define df7k cs7k
#define ds7k ef7k
#define es7k f7k 
#define ff7k e7k  
#define gf7k fs7k
#define gs7k af7k
#define as7k bf7k
#define bs7k  c8k 
#define cf8k b7k  

#define bs7k c8k
#define df8k cs8k
#define ds8k ef8k
#define es8k f8k 
#define ff8k e8k  
#define gf8k fs8k
#define gs8k af8k
#define as8k bf8k
#define bs8k  c9k 
#define cf9k b8k  

#define bs8k c9k
#define df9k cs9k
#define ds9k ef9k
#define es9k f9k 
#define ff9k e9k  
#define gf9k fs9k

#endif KEYNUMS_H



