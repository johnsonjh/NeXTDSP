/*
  UnitGenerator.h
  Copyright 1988, NeXT, Inc.
  DEFINED IN: The Music Kit
  */
#import <objc/Object.h>
#import "orch.h"

typedef struct _MKUGArgStruct {   /* Used to represent Unit Generator args */
    MKOrchAddrStruct addrStruct;  /* Specifies location of arg. */
    DSPMemorySpace addressMemSpace;/* For address-valued arguments, 
                      space where the DSP code assumes the 
                      address is or DSP_MS_N */
    DSPLongDatum curVal;           /* The most recently poked value of arg.
                      If arg is not long, low order word
                      is ignored. (Used by optimizer)  */
    BOOL initialized;              /* Argument set yet? (Used by optimizer) */
    int type;                      /* Reserved. */
} MKUGArgStruct;

#import "dspwrap.h"

@interface UnitGenerator : Object
/*
 * 
 * UnitGenerator is an abstract class; each subclass provides a
 * particular music synthesis operation or function.  A UnitGenerator
 * object represents a DSP unit generator, a program that runs on the
 * DSP.
 * 
 * You never create UnitGenerator objects directly in an application,
 * they can only be created by the Orchestra through its
 * allocUnitGenerator: method.  UnitGenerators are typically owned by a
 * SynthPatch, an object that configures a set of SynthData and
 * UnitGenerator objects into a DSP software instrument.  The Music Kit
 * provides a number of UnitGenerator subclasses that can be configured
 * to create new SynthPatch classes.
 * 
 * Most of the methods defined in the UnitGenerator class are subclass
 * responsiblities or are provided to help define the functionality of a
 * subclass.  The most important of these are runSelf, idleSelf, and
 * finishSelf.  These methods implement the behavior of the object in
 * response to the run, finish, and idle messages, respectively.
 * 
 * In addition to implementing the subclass responsibility methods, you
 * should also provide methods for poking values into the memory
 * arguments of the DSP unit generator that the UnitGenerator represents.
 * For example, an oscillator UnitGenerator would provide a setFreq:
 * method to set the frequency of the unit generator that's running on
 * the DSP.
 * 
 * UnitGenerator subclasses are created from DSP macro code.  The utility
 * dspwrap turns a DSP macro into a UnitGenerator master class,
 * implementing some of the subclass responsibility methods.
 * 
 * It also creates a number of classes that inherit from your
 * UnitGenerator subclass; these are called leaf classes.  A leaf class
 * represents a specific memory space configuration on the DSP.  For
 * example, OnePoleUG is a one-pole filter UnitGenerator master class
 * provided by the Music Kit.  It has an input and an output argument
 * that refer to either the x or the y memory spaces on the DSP.  To
 * provide for all memory space configurations, dspwrap creates the leaf
 * classes OnePoleUGxx, OnePoleUGxy, OnePoleUGyx, and OnePoleUGyy.
 * 
 * You can modify a master class (the setFreq: method mentioned above
 * would be implemented in a master class), but you never create an
 * instance of one.  UnitGenerator objects are always instances of leaf
 * classes.
 * 
 * CF: SynthData, SynthPatch, Orchestra
 */
{
    id synthPatch;      /* The SynthPatch that owns this object, if any. */
    id orchestra;       /* The Orchestra on which the object is allocated. */
    unsigned short _reservedSynthElement1;
    unsigned short _reservedSynthElement2;
    id _reservedSynthElement3;
    BOOL _reservedSynthElement4;
    void *_reservedSynthElement5;
    BOOL isAllocated;   /* YES if allocated */
    MKUGArgStruct *args;   
    MKSynthStatus status;
    MKOrchMemStruct relocation;
    MKLeafUGStruct *_reservedUnitGenerator1;
    id _reservedUnitGenerator2;
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

+(MKMasterUGStruct *) masterUGPtr; 
 /* TYPE: Querying; Returns the receiver's master structure. 
  * Returns the receiver's master structure.  A subclass responsility,
  * this method is automatically generated by dspwrap.
  */

+(MKLeafUGStruct *) classInfo; 
 /* TYPE: Querying; Returns the receiver's leaf structure.
  * Returns the receiver's leaf structure.  A subclass responsility, this
  * method is automatically generated by dspwrap.
  */

+(unsigned ) argCount; 
 /* TYPE: Querying; Returns the number of memory arguments in the receiver's DSP code.
  */
   
- moved; 
 /* TYPE: Modifying; Sent if the Orchestra had to move the receiver during compaction.
  * You never invoke this method.  It's invoked by the Orchestra if it had
  * to move the receiver during compaction.  A subclass can override this
  * method to perform special behavior.  The default does nothing.  The
  * return value is ignored.
  */

+(BOOL)shouldOptimize:(unsigned) arg;
 /* TYPE: Querying; YES if arg should be optimized.
  * A subclass can override this method to reduce the command stream on an
  * argument-by-argument basis, returning YES if arg should be optimized,
  * NO if it shouldn't.  The default implementation always returns NO.
  * 
  * Optimization means that if the argument is set to the same value twice,
  * the second setting is supressed.  You should never optimize an argument that
  * the receiver's DSP code itself might change.
  * 
  * Argument optimization applies to the entire class--all instances of 
  * the UnitGenerators leaf classes inherit an argument's optimization--
  * and it can't be changed during a performance.
  */

- init;
 /* TYPE: Creating
  * You never invoke this method; it's sent when the receiver is is
  * created, after its code is loaded.  If this method returns nil, the
  * receiver is automatically freed by the Orchestra.  A subclass
  * implementation should send [super init] before doing its own
  * initialization and should immediately return nil [super init]
  * returns nil.  The default implementation returns self.
  */

- initialize;
 /* TYPE: Creating; Obsolete.
  */

- run;
 /* TYPE: Modifying; Tells the receiver to start running.
Starts the receiver by sending [self runSelf]
and then sets its status to MK_running.  You never subclass 
this method; runSelf provides subclass runtime instructions.
A UnitGenerator must be sent run 
before it can be used.
  */

- runSelf; 
 /* TYPE: Modifying; Subclass run routine; default does nothing.
 * Subclass implementation of this method provides instructions for 
 * making the object's DSP code usable (as defined by the subclass).
 * You never invoke this method directly, it's invoked automatically
 * by the run method.
 * The default does nothing and returns the receiver.
 */

-(double ) finish; 
 /* TYPE: Modifying; Tells the receiver to finish running.
 * Finishes the receiver's activity by sending finishSelf and
 * then sets its status to MK_finishing.  
 * You never subclass this method; finishSelf
 * provides subclass finishing instructions.  Returns the value of 
 * [self finishSelf], which is taken as the amount of 
 * time, in seconds, before the receiver can be idled.
 */

-(double ) finishSelf; 
 /* TYPE: Modifying; Subclass finish routine; default does nothing.
 * A subclass may override this method to provide instructions for
 * finishing.  Returns the amount of time needed to finish;
 * The default returns 0.0.
 */

- idle;
 /* TYPE: Modifying; Tells the receiver to become idle.
 * Idles the receiver by sending [self idleSelf] and
 * then sets its status to MK_idle.
 * You never subclass this method; idleSelf provides subclass 
 * idle instructions.
 * The idle state is defined as the UnitGenerator's producing no output.
 */

- idleSelf; 
 /* TYPE: Modifying; Subclass idle routine; default does nothing.
 * A subclass may override this method to provide instructions for
 * idling.  The default does nothing and returns the receiver.
 * Most UnitGenerator subclasses implement idleSelf to patch their 
 * outputs to sink, a location that, by convention, nobody reads. 
 * UnitGenerators that have inputs,
 * such as Out2sumUG, implement idleSelf to patch their
 * inputs to zero, a location that always holds the value 0.0.
 */

-(int ) status; 
 /* TYPE: Querying; Returns the receiver's status.
 * Returns the receiver's status, one of
 * MK_idle, MK_running, and MK_finishing.
 */

-(MKOrchMemStruct *) relocation; 
 /* TYPE: Querying; Returns a pointer to the receiver's DSP location structure.
 * Returns a pointer to the structure that describes the receiver's
 * location on the DSP.  You can access the fields of the structure
 * without caching it first, for example:
 * 
 * [aUnitGenerator relocation]->pLoop
 *
 * returns the starting location of the receiver's pLoop code.
 */

-(BOOL ) runsAfter:aUnitGenerator; 
 /* TYPE: Querying; YES if the argument runs after the receiver. 
 * Returns YES if the receiver is executed after aUnitGenerator.
 * Execution order is determined by comparing the objects' 
 * pLoop addresses.
 */

-(unsigned ) argCount; 
 /* TYPE: Querying; Returns the number of memory arguments in the receiver's DSP code.
 * Returns the number of memory arguments in the receiver's DSP code.
 * The same value is returned by the argCount class method.
 */

-(MKLeafUGStruct *) classInfo; 
 /* TYPE: Querying; Returns a pointer to the receiver's leaf structure.
 * Returns a pointer to the receiver's leaf structure.
 * The same structure pointer is returned by the classInfo class 
 * method. 
 */

-(MKOrchMemStruct *) resources; 
 /* TYPE: Querying; Returns a pointer to receiver's memory requirements structure.
 * Return a pointer to the structure that describes the receiver's 
 * memory requirements.  Each field of the structure represents a particular
 * Orchestra memory segment; its value represents the number of words that the 
 * segment requires.
 */ 

+(char * ) argName:(unsigned )argNum; 
 /* TYPE: Querying; Returns the name of the receiver's argNum'th DSP code argument.
 * Returns the name of the receiver's argNum'th DSP code argument, as
 * declared in the DSP unit generator source code.  The name isn't copied.
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
 * Deallocates the receiver and frees its SynthPatch, if any.
 * Returns nil.
 */

-(BOOL ) isFreeable; 
 /* TYPE: Querying; YES if the receiver may be freed.
 * Invoked by the Orchestra to determine whether the receiver may
 * be freed.  Returns YES if it can, NO if it can't.
 * (A UnitGenerator can be freed if it isn't currently allocated
 * or its SynthPatch can be freed.)
 */

- synthPatch; 
 /* TYPE: Querying; Returns the receiver's SynthPatch.
 * Returns the SynthPatch that the receiver is part of, if any.
 */

-(BOOL ) isAllocated; 
 /* TYPE: Querying;  YES if the receiver has been allocated.
 * Returns YES if the receiver has been allocated (by its Orchestra),
 * NO if it hasn't.
 */

- setDatumArg:(unsigned )argNum to:(DSPDatum )val; 
 /* TYPE: Modifying; Sets datum-valued argument argNum to val.
 * Sets the datum-valued argument argNum to val.  
 * If argNum is an L-space argument (two 24-bit words), its
 * high-order word is set to val and its low-order word is cleared.
 * If argNum (as an index) is out of bounds, an error is
 * generated and nil is returned.  Otherwise returns the receiver.
 * This is ordinarily invoked by a subclass.
 */

- setDatumArg:(unsigned)argNum toLong:(DSPLongDatum *)val;
 /* TYPE: Modifying; Sets datum-valued argument argNum to val.
 * Sets the datum-valued argument argNum to val.  
 * If argNum isn't an L-space argument
 * (it can't accommodate a 48-bit value) its value is set to the high 24-bits of
 * val.
 * If argNum (as an index) is out of bounds, an error is
 * generated and nil is returned.  Otherwise returns the receiver.
 * This is ordinarily only invoked by a subclass.
 */

- setAddressArg:(unsigned )argNum to:memoryObj; 
 /* TYPE: Modifying; Sets the address-valued argument argNum to memoryObj.
 * Sets the addresst-valued argument argNum to memoryObj.
 * If argNum (as an index) is out of bounds, an error is
 * generated and nil is returned.  Otherwise returns the receiver.
 * This is ordinarily only invoked by a subclass.
 */

- setAddressArg:(unsigned )argNum toInt:(DSPAddress)address;
 /* TYPE: Modifying; Sets the address-valued argument argNum to memoryObj.
 * Sets the addresst-valued argument argNum to address.
 * If argNum (as an index) is out of bounds, an error is
 * generated and nil is returned.  Otherwise returns the receiver.
 * This is ordinarily only invoked by a subclass.
 */

- setAddressArgToSink:(unsigned )argNum; 
 /* TYPE: Modifying; Sets the address-valued argument argNum to the sink patchpoint.
 * Sets the address-valued argument argNum to the sink patchpoint.
 * (Sink is a location which, by convention, is never read.) 
 * If argNum (as an index) is out of bounds, an error is
 * generated and nil is returned.  Otherwise returns the receiver.
 * This is ordinarily only invoked by a subclass.
 */

- setAddressArgToZero:(unsigned )argNum; 
 /* TYPE: Modifying; Sets the address-valued argument argNum to the zero patchpoint.
 * Sets the address-valued argument argNum to a zero patchpoint.
 * (A zero patchpoint is a location with a constant 0 value; by convention
 * the patchpoint is never written to.)
 * If argNum (as an index) is out of bounds, an error is
 * generated and nil is returned.  Otherwise returns the receiver.
 * This is ordinarily only invoked by a subclass.
 */

+(DSPMemorySpace ) argSpace:(unsigned )argNum; 
 /* TYPE: Querying; Returns the memory space to or from which the argument argNum reads or writes.
 * Returns the memory space to or from which the address-valued 
 * argument argNum 
 * read or writes.
 * If argNum isn't an address-valued argument, returns DSP_MS_N.
 */

- freeSelf;
/* You implement this to do any special behavior before the object is freed. 
   For example, you might want to release locally-allocated SynthData. */

+enableErrorChecking:(BOOL)yesOrNo;
 /* TYPE: Modifying; Sets whether error checking code is invoked.
 * Sets whether various error checks are done, such as verifying that
 * UnitGenerator arguments and Synthdata memory spaces are correct.
 * The default is NO. You should send enableErrorChecking:YES when
 * you are debugging UnitGenerators or SynthPatches, then disable it when
 * your application is finished. */

-(int)referenceCount;
 /* TYPE: Querying; Returns the reference count. 
 * If this object is installed in its Orchestra's shared table, returns the
 * number of objects that have allocated it. Otherwise returns 1 if it is 
 * allocated, 0 if it is not. */

/* Functions that are equivalent to above methods, for speed. The first
   argument is assumed to be an instance of class UnitGenerator. */
id MKSetUGDatumArg(id self,unsigned argNum,DSPDatum val);
id MKSetUGDatumArgLong(id self,unsigned argNum,DSPLongDatum *val);
id MKSetUGAddressArg(id self,unsigned argNum,id memoryObj);
id MKSetUGAddressArgToInt(id self,unsigned argNum,DSPAddress addr);

 /* -read: and -write: 
  * Note that archiving is not supported in the UnitGenerator object, since,
  * by definition the UnitGenerator instance only exists when it is resident on
  * a DSP.
  */

@end




