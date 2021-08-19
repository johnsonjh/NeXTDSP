/*
  SynthData.h
  Copyright 1988, NeXT, Inc.
  
  DEFINED IN: The Music Kit
  */

#import <objc/Object.h>

@interface SynthData : Object
/* 
 * 
 * SynthData objects represent DSP memory that's used in music synthesis.
 * For example, you can use a SynthData object to load predefined data
 * for wavetable synthesis or to store DSP-computed data to create a
 * digital delay.  Perhaps the most common use of SynthData is to create
 * a location through which UnitGenerators can pass data.  This type of
 * SynthData object is called a patchpoint.  For example, in frequency
 * modulation an oscillator UnitGenerator writes its output to a
 * patchpoint which can then be read by another oscillator as its
 * frequency input.
 * 
 * You never create SynthData objects directly in an application, they
 * can only be created by the Orchestra through its
 * allocSynthData:length: or allocPatchpoint: methods.  SynthData objects
 * are typically owned by a SynthPatch, an object that configures a set
 * of SynthData and UnitGenerator objects into a DSP software instrument.
 * 
 * The methods setData: and setConstant: are used to load a SynthData
 * object with data from an array or as a constant, respectively.  These
 * methods are simple versions of the more thorough methods
 * setData:length:offset: and setConstant:length:offset:, which allow you
 * to load an arbitrary amount of data into any portion of the
 * SynthData's memory.  The data in a SynthData object, like all DSP data
 * used in music synthesis, is 24-bit fixed point words (data type
 * DSPDatum).  You can declare a SynthData to be read-only by sending it
 * the message setReadOnly:YES.  You can't change the data in a read-only
 * SynthData object.
 * 
 * An instance of SynthData consists of an MKOrchAddrStruct, a structure
 * that describes the DSP location of the object's data, and a length
 * instance variable, an integer value that measures the size of the data
 * in DSPDatum words.  However, it doesn't contain a copy of the memory
 * itself.  When you load data into a SynthData, it's instantly sent to
 * the DSP device driver.
 * 
 * DSP memory allocation and management is explained in the Orchestra
 * class description; many of the return types used here, such as
 * DSPAddress and DSPMemorySpace, are described in Orchestra.  In
 * general, the design of the Orchestra makes intimate knowledge of the
 * details of the DSP unnecessary.
 * 
 * CF: SynthPatch, Orchestra, UnitGenerator
 */
{
    id synthPatch;      /* The SynthPatch that owns this object (if any). */
    id orchestra;       /* The orchestra on which the object is allocated. */
    unsigned short _reservedSynthElement1;
    unsigned short _reservedSynthElement2;
    id _reservedSynthElement3;
    BOOL _reservedSynthElement4;
    void *_reservedSynthElement5;
    int length;                /* Length of allocated memory in words. */
    MKOrchAddrStruct orchAddr; /* Structure that directly represents DSP 
                                  memory. */
    BOOL readOnly;             /* YES if the object's data is read-only. */
    MKOrchMemStruct _reservedSynthData1;
}
 
 /* METHOD TYPES
  * Modifying the object
  * Querying the object
  * Running the object
  */


+new;
+allocFromZone:(NXZone *)zone;
+alloc;
-copy;
-copyFromZone:(NXZone *)zone;
 /* These methods are overridden to return [self doesNotRecognize]. 
    You never create, free or copy UnitGenerators directly. These operations
    are always done via an Orchestra object. */

- free; 
 /* Same as [self dealloc]. */

- clear; 
 /* TYPE: Modifying; Clears the receiver's memory but doesn't deallocate it.
  */   

-(int ) length; 
 /* TYPE: Querying; Returns the size (in words) of the receiver's memory block.
 */

-(DSPAddress ) address; 
 /* TYPE: Querying; Returns the DSP address of the receiver's memory block.
 */   

-(DSPMemorySpace ) memorySpace; 
 /* TYPE: Querying; Returns the DSP space from which the receiver's memory block is allocated.
  */

-(MKOrchAddrStruct *) orchAddrPtr; 
 /* TYPE: Querying;  Returns a pointer to the receiver's address structure.
 */

- setData:(DSPDatum *)dataArray length:(int )len offset:(int )off; 
 /* TYPE: Modifying; Loads the array dataArray into the receiver's memory.
 * Loads (at most) len words of data from dataArray 
 * into the receiver's memory, starting at location off words from
 * the beginning of the receiver's memory block.
 * If off + len is greater than the receiver's length (as returned
 * by the length method), or if the data couldn't otherwise be loaded,
 * the error MK_synthDataLoadErr is generated and nil is returned.
 * Otherwise returns the receiver.
 */

-setShortData:(short *)dataArray length:(int)len offset:(int)off;
 /* TYPE: Modifying; Loads the short array dataArray, right-justified.
 * Loads (at most) len words of data from dataArray 
 * into the receiver's memory, right justified, starting at location 
 * off words from the beginning of the receiver's memory block.
 * If off + len is greater than the receiver's length (as returned
 * by the length method), or if the data couldn't otherwise be loaded,
 * the error MK_synthDataLoadErr is generated and nil is returned.
 * Otherwise returns the receiver.
 */

- setData:(DSPDatum *)dataArray; 
 /* TYPE: Modifying; Fills the receiver's memory with the array dataArray.
 * Loads dataArray into the receiver's memory.  Implemented
 * as (and returns the value of)
 *
 * [self setData:dataArray length:length offset:0];
 *
 * where the second argument is the instance variable length.
 * This assumes that dataArray is the same length as the receiver.
 */

- setShortData:(short *)dataArray; 
 /* TYPE: Modifying; Fills the receiver's memory with the short array dataArray.
 * Loads dataArray into the receiver's memory, right justified. Implemented
 * as (and returns the value of)
 *
 * [self setShortData:dataArray length:length offset:0];
 *
 * where the second argument is the instance variable length.
 * This assumes that dataArray is the same length as the receiver.
 */

- setToConstant:(DSPDatum )value length:(int )len offset:(int )off; 
 /* TYPE: Modifying; Loads the constant value into the receiver's memory.
 * Similar to setData:length:offset:, but loads the constant value
 * rather than an array; see setData:length:offset: for details.
 */

- setToConstant:(DSPDatum )value; 
 /* TYPE: Modifying; Fills the receiver's memory with the constant value.
 * Fills the receiver's memory with the constant value.  
 * Implemented as (and returns the value of)
 *
 * [self setToConstant:value length:length offset:0];
 *
 * where the second argument is the instance variable length.
 */
   
- run;
 /* TYPE: Running; Does nothing; provided for compatibility with UnitGenerator.
 * This does nothing and returns the receiver.  It's provided for 
 * compatibility with UnitGenerator; specifically, it allows a SynthPatch
 * to send run to all its SynthElement objects without regard 
 * for their class.
 */

- idle;
 /* TYPE: Running; Does nothing; provided for compatibility with UnitGenerator.
 * This does nothing and returns the receiver.  It's provided for 
 * compatibility with UnitGenerator; specifically, it allows a SynthPatch
 * to send idle to all its SynthElement objects without regard 
 * for their class.
 */

- (double)finish;
 /* TYPE: Running; Does nothing; provided for compatibility with UnitGenerator.
 * This does nothing and returns 0.  It's provided for 
 * compatibility with UnitGenerator; specifically, it allows a SynthPatch
 * to send finish to all its SynthElement objects without regard 
 * for their class.
 */

+ orchestraClass;
 /* TYPE: Querying; Returns the Orchestra class object.
 * This method always returns the Orchestra class.  It's provided for
 * applications that extend the Music Kit to use other synthesis hardware. 
 * If you're using more than one type of hardware, you should create
 * a subclass of UnitGenerator for each. 
 * The default hardware is that represented by Orchestra, the DSP56001.
 */

- orchestra; 
 /* TYPE: Querying; Returns the receiver's Orchestra object.
 */

- dealloc;
 /* TYPE: Modifying; Deallocates the receiver.
 * Deallocates the receiver and frees its SynthPatch, if any. Returns nil.
 */

-(BOOL) isAllocated;
 /* TYPE: Querying; Always returns YES.
 * Provided for compatability with UnitGenerator. Always returns YES, 
 * since deallocated SynthDatas are freed immediately. */

-(BOOL ) isFreeable; 
 /* TYPE: Querying; YES if the receiver may be freed.
 * Invoked by the Orchestra to determine whether the receiver may
 * be freed.  Returns YES if it can, NO if it can't.
 * (A SynthData can be freed if its a member of a Synthpatch that 
 * can be freed.)
 */

- synthPatch; 
 /* TYPE: Querying; Returns the receiver's SynthPatch.
 * Returns the SynthPatch that the receiver is part of, if any.
 */

- setReadOnly:(BOOL)readOnlyFlag;
 /* TYPE: Modifying; Declares the receiver as read-only if readOnlyFlag is YES.
 * Sets the receiver to read-only if readOnlyFlag 
 * is YES and read-write if it's NO.
 * The default access for a SynthData object is read-write.
 * Returns the receiver.
 * The Orchestra automatically creates some read-only SynthData objects
 * (SineROM, MuLawROM, and the zero and sink patchpoints) that ignore
 * this method.  
 */ 

- (BOOL)readOnly;
 /* TYPE: Querying; Returns YES if the receiver is read-only. */

-(int)referenceCount;
 /* TYPE: Querying; Returns the reference count. 
 * If this object is installed in its Orchestra's shared table, returns the
 * number of objects that have allocated it. Otherwise returns 1. */

 /* -read: and -write: 
  * Note that archiving is not supported in the SynthData object, since,
  * by definition the SynthData instance only exists when it is resident on
  * a DSP.
  */


@end



