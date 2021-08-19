#ifdef SHLIB
#include "shlib.h"
#endif

/*
  PatchTemplate.m
  Copyright 1988, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

       01/02/90/daj - Deleted a comment.		      
       03/13/90/daj - Changes to support new categories for private methods.
       03/21/90/daj - Added archiving.
       04/21/90/daj - Small mods to get rid of -W compiler warnings.
       04/25/90/daj - Added CHECKCLASS to make sure that the right class is
                      returned when inheritance is used in SynthPatch design.
       04/27/90/daj - Removed checks for _MKClassOrchestra, since we're a
                      shlib now so Orchestra will always be there.
       08/27/90/daj - Added zone support API.
*/
#import "_musickit.h"

#import <objc/Storage.h>
#import "_Orchestra.h"
#import "_SynthPatch.h"
#import "_UnitGenerator.h"

#import "_PatchTemplate.h"
@implementation PatchTemplate:Object
/* 
  PatchTemplate is a recipe for building a particular kind of SynthPatch.
  The template is created with the PatchTemplate class method +new
  and configured with the basic methods 

  -to:(unsigned)anObjInt sel:(SEL)aSelector arg:(unsigned)anArgInt
  -(unsigned)addUnitGenerator:(id)aUGClass ordered:(BOOL)isOrdered
  -(unsigned)addSynthData:(MKOrchMemSegment)segment length:(unsigned)len
  -(unsigned)addPatchpoint:(MKOrchMemSegment)segment 
   
  The template consists of "ordered" UnitGenerator factories,
  "unordered" UnitGenerator factories, "data memory blocks", and
  "message requests". (The meaning of the terms is indicated below.)  These
  are added to the template using the three methods shown above. In the
  case of "ordered" UnitGenerators, the order used is the order of the
  -addUnitGenerator: messages. -addUnitGenerator:ordered:, -addSynthData:length:,
  and -addPatchpoint: return an int value to be used as an argument to 
  sendSel:to:with: or when referencing UnitGenerators in the SynthPatch.
  
  When UnitGenerators are connected up, it usually doesn't matter if
  there is a one-tick delay involved in the interconnection.  If it does
  not matter one way or the other, you should specify the UnitGenerator
  as an "unordered" UnitGenerator. However, if it is essential that no
  pipe-line delay be incurred, you should specify the two unit
  generators in the correct order as "ordered" UnitGenerators.
  Similarly, if it is essential that exactly one pipe-line delay be
  incurred, you should specify the two UnitGenerators in the reverse
  order in the "ordered" UnitGenerator list. 
  
  Each data block used privately in the SynthPatch is allocated
  by specifying the length and memory segment of that data block to the
  PatchTemplate. The instances allocated are SynthData instances. 
  
  Finally, the message requests of the Template are specified with the
  to:sel:arg: method. This mechanism is used primarily to specify the
  interconnections of the UnitGenerators.  
  The to: and with: arguments are one of the values 
  returned by addUnitGenerator:ordered: or addSynthData:length:. 
  These connections are made automatically by the -initialize SynthPatch 
  method. When the initialize method is sent to the SynthPatch, 
  each of the connections is made in the order the to:sel:arg: messages 
  were sent.

  It is important to point out that PatchTemplates are considered different
  by the Orchestra, even if their contents are identical. A PatchTemplate should not
  be changed once it has been used during a Musickit performance. 

  PatchTemplate should never be freed.
  */
{
    id _reservedPatchTemplate1;  
    id _reservedPatchTemplate2;  
    id *_reservedPatchTemplate3;
    unsigned int _reservedPatchTemplate4;
    void *_reservedPatchTemplate5;
}

#define _elementStorage _reservedPatchTemplate1
/* Storage class object of template entries */
#define _connectionStorage _reservedPatchTemplate2
/* Storage class object of connection info */ 
#define _deallocatedPatches _reservedPatchTemplate3
/* If Orchestra is loaded, this is an array of Lists 
   of deallocated patches, one per DSP.  */
#define _eMemSegments _reservedPatchTemplate4
/* External memory segment bit vector */

typedef struct _templateEntry {
    id class;             /* Which class. */
    unsigned short type;  /* See above */
    unsigned short seg;   /* Used only for data memory. MKOrchMemSegment. */
    unsigned length;      /* " Length of data. */ 
    } templateEntry;

typedef struct _connection { 
	unsigned _toObjectOffset;	       
	unsigned _argObjectOffset;
	SEL _aSelector;       
	IMP _methodImp;
    } connection;


#define CONNSIZE	(sizeof(connection))
#define CONNDESCR	"{ii**}" /* FIXME SEL and IMP are handled funny */
#define ENTRYSIZE	(sizeof(templateEntry))
#define ENTRYDESCR	"{@SSI}"

+new
{
    self = [super allocFromZone:NXDefaultMallocZone()];
    [self init];
    return self;
}

-init
  /* Creates a new PatchTemplate instance. */
{
    [super init];
    _deallocatedPatches = [_MKClassOrchestra() _addTemplate:self];
    _connectionStorage = 
      [Storage newCount:0 elementSize:CONNSIZE description:CONNDESCR];
    _elementStorage = 
      [Storage newCount:0 elementSize:ENTRYSIZE description:ENTRYDESCR];
    return self;
}

#define VERSION2 2

+initialize
{
    if (self != [PatchTemplate class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

- write:(NXTypedStream *) aTypedStream
  /* TYPE: Archiving; Writes object.
     You never send this message directly.  
     */
{
    [super write:aTypedStream];
    NXWriteTypes(aTypedStream,"@@i",&_elementStorage,&_connectionStorage,
		 &_eMemSegments);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked via NXReadObject(). 
     See write:. */
{
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"PatchTemplate") == VERSION2) 
      NXReadTypes(aTypedStream,"@@i",&_elementStorage,&_connectionStorage,
		  &_eMemSegments);
    return self;
}

-awake
{
    [super awake];
    _deallocatedPatches = [_MKClassOrchestra() _addTemplate:self];
    return self;
}

-copy
{
    return [self copyFromZone:[self zone]];
}

-copyFromZone:(NXZone *)zone
  /* Creates a new PatchTemplate that's a copy of the receiver, containing
     the same connections and entries. */
{
    PatchTemplate *newObj = [super copyFromZone:zone];
    int elements,connections;
    _deallocatedPatches = [_MKClassOrchestra() _addTemplate:newObj];
    connections = [_connectionStorage count]; 
    elements = [_elementStorage count]; 
    newObj->_connectionStorage = [Storage newCount:connections
				elementSize:CONNSIZE description:CONNDESCR];
    newObj->_elementStorage = [Storage newCount:elements
			     elementSize:ENTRYSIZE description:ENTRYDESCR];
    memcpy((templateEntry *)[newObj->_elementStorage elementAt:0],
	   (templateEntry *)[_elementStorage elementAt:0],
	   ENTRYSIZE * elements);
    memcpy((templateEntry *)[newObj->_connectionStorage elementAt:0],
	   (templateEntry *)[_connectionStorage elementAt:0],
	   ENTRYSIZE * connections);
    return newObj;
}

-(unsigned)synthElementCount
  /* Returns the number of entries in the template. */
{
    return [_elementStorage count];
}

-to:(unsigned)toObjInt sel:(SEL)aSelector arg:(unsigned)withObjInt
  /* Adds a request to send aSelector to the entry specified by toObjInt
     with the argument as the entry specified by withObjInt. For example,
     if you say 

     unsigned osc = [tmpl addUnitGenerator:OscgUG]; 
     unsigned  patchPoint = [tmpl addPatchpoint:MK_xPatch];
     [tmpl to:osc sel:@selector(setOutput:) arg:patchPoint];

     then later, when the SynthPatch is built, the message

     [[self at:osc] setOutput:[self at:patchPoint]];

     will be sent. If toObjInt or withObjInt is invalid, returns nil, else self.
     */
{
    connection conn;
    int i;
    conn._toObjectOffset = toObjInt;
    conn._argObjectOffset = withObjInt;
    conn._aSelector = aSelector;
    i = [_elementStorage count];
    if ((toObjInt < i) && (withObjInt < i))
      conn._methodImp = 
	[((templateEntry *)[_elementStorage elementAt:conn._toObjectOffset])->
	 class instanceMethodFor:conn._aSelector];
    else 
      return nil;
    [_connectionStorage insert:((char *)&conn) at:[_connectionStorage count]];
    return self;
}

#define ORDERED   ((short)1)
#define UNORDERED ((short)2)
#define SYNTHDATA ((short)3)
#define PATCHPOINT ((short)4)

static unsigned addEl(PatchTemplate *self,templateEntry *newEntry)
{
    unsigned curIndex = [self->_elementStorage count];
    /* Count is num elements. But we pass an index so need to subtract 1. */ 
    [self->_elementStorage insert:(char *)newEntry at:curIndex];
    return curIndex; 
}

-(unsigned)addUnitGenerator:(id)aUGClass ordered:(BOOL)isOrdered
  /* Adds a UnitGenerator or PatchPoint class to the receiver. If isOrdered
     is NO, the ordering of the UnitGenerator in memory is considered
     irrelevant. It is more efficient, from the standpoint of memory 
     thrashing, to set isOrdered to NO. However, it makes the job of 
     writing SynthPatches tricker, since the designer may need to ask each
     UnitGenerator if it runs after or before the others. */
{
    templateEntry newEntry;
    newEntry.class = aUGClass;
    newEntry.type = (isOrdered) ? ORDERED : UNORDERED;
    return addEl(self,&newEntry);
}

-(unsigned)addUnitGenerator:aUGClass
  /* Same as [self addUnitGenerator:aUGClass ordered:YES]; */
{
    return [self addUnitGenerator:aUGClass ordered:YES];
}

-(unsigned)addSynthData:(MKOrchMemSegment)segment length:(unsigned)len
  /* Adds a request for a data memory segment of the specified segment type
     and length. */
{
    templateEntry newEntry;
    newEntry.class = [SynthData class];
    newEntry.type = SYNTHDATA;
    newEntry.seg = (unsigned short)segment;
    newEntry.length = len;
    return addEl(self,&newEntry);
}

-(unsigned)addPatchpoint:(MKOrchMemSegment)segment
  /* Adds a request for a data memory segment of the specified segment type
     and length. */
{
    templateEntry newEntry;
    newEntry.class = [SynthData class];
    newEntry.type = PATCHPOINT;
    newEntry.seg = (unsigned short)segment;
    return addEl(self,&newEntry);
}

id _MKDeallocatedSynthPatches(PatchTemplate *templ,int orchIndex)
{
    return templ->_deallocatedPatches[orchIndex];
}

BOOL _MKIsClassInTemplate(PatchTemplate *templ,id factObj)
{
   /* Returns YES if factObj is present in templ as a unit generator,
      ordered or unordered. */
   register templateEntry *arrEnd,*el;
   el = (templateEntry *)([templ->_elementStorage elementAt:0]);
   for (arrEnd = el + [templ->_elementStorage count]; el < arrEnd; el++) 
     if ((el->class) == factObj) 
       return YES;
   return NO;
}

void _MKEvalTemplateConnections(PatchTemplate *templ,id synthElements)
{
    register connection *conn;
    register unsigned n;
    id *arr = NX_ADDRESS(synthElements);
    conn = (connection *)([templ->_connectionStorage elementAt:0]);
    for (n = [templ->_connectionStorage count]; n--; conn++) 
      (*conn->_methodImp)(arr[conn->_toObjectOffset],conn->_aSelector,
			  arr[conn->_argObjectOffset]);
}

unsigned _MKGetTemplateEMemUsage(PatchTemplate *templ)
{
    return templ->_eMemSegments;
}

void _MKSetTemplateEMemUsage(PatchTemplate *templ,MKOrchMemStruct *reso)
{
    if (templ->_eMemSegments == MAXINT) {
	if (reso->xData) 
	  templ->_eMemSegments |= (1 << ((unsigned)(MK_xData)));
	if (reso->yData)
	  templ->_eMemSegments |= (1 << ((unsigned)(MK_yData)));
	if (reso->pSubr)
	  templ->_eMemSegments |= (1 << ((unsigned)(MK_pSubr)));
    }
}

#define CHECKCLASS 1

id _MKAllocSynthPatch(PatchTemplate *templ,id synthPatchClass,id anOrch,
		      int orchIndex)
{
    id aPatch;
#if CHECKCLASS
    if (templ) {
    	int n = [templ->_deallocatedPatches[orchIndex] count];
	id *ptr = NX_ADDRESS(templ->_deallocatedPatches[orchIndex]) + n; 
			/* points 1 beyond last object */
	aPatch = nil;
	while ((n-- > 0) && (aPatch == nil)) { 
	    /* March down List (from end toward beginning) looking for a 
	       class match. Normally, the very first one we check will 
	       match. */
	    if ([*--ptr class] == synthPatchClass) {
		[templ->_deallocatedPatches[orchIndex] removeObjectAt:n];
		aPatch = *ptr;
	    }	
	}
	if (aPatch) {
	    if (_MK_ORCHTRACE(anOrch,MK_TRACEORCHALLOC))
	      _MKOrchTrace(anOrch,MK_TRACEORCHALLOC,
			   "allocSynthPatch returns %s_%d",
			   [synthPatchClass name],aPatch);
	    [aPatch _allocate]; /* Tell it it's allocated */
	    return aPatch;
	}
    }
#else
    if (templ && 
	(aPatch = [templ->_deallocatedPatches[orchIndex] removeLastObject])) {
	if (_MK_ORCHTRACE(anOrch,MK_TRACEORCHALLOC))
	  _MKOrchTrace(anOrch,MK_TRACEORCHALLOC,
		       "allocSynthPatch returns %s_%d",
		       [synthPatchClass name],aPatch);
	[aPatch _allocate]; /* Tell it it's allocated */
	return aPatch;
    }
#endif
    /* If no deallocated patches, we try and allocate a new one. */
    [anOrch beginAtomicSection];
    aPatch = [synthPatchClass _newWithTemplate:templ inOrch:anOrch index:
	      orchIndex];
    if (!templ)
      return aPatch;
    if (_MK_ORCHTRACE(anOrch,MK_TRACEORCHALLOC))
      _MKOrchTrace(anOrch,MK_TRACEORCHALLOC,
		   "allocSynthPatch building %s_%d...",
		   [synthPatchClass name],aPatch);
    
    {
	register templateEntry *arrEnd,*el;
#if _MK_MAKECOMPILERHAPPY
	id anOrderedUG = nil;
	id aSE = nil;
#else
	id anOrderedUG,aSE;
#endif
	BOOL firstOrdered;
	
	firstOrdered = YES;
	el = (templateEntry *)([templ->_elementStorage elementAt:0]);
	for (arrEnd = el + [templ->_elementStorage count]; 
	     el < arrEnd; 
	     el++) {
	    switch (el->type) {
	      case ORDERED:
		if (firstOrdered) {
		    anOrderedUG = [anOrch allocUnitGenerator:el->class]; 
		    firstOrdered = NO;
		}
		else 
		  anOrderedUG = [anOrch allocUnitGenerator:el->class 
			       after:anOrderedUG];
		aSE = anOrderedUG;
		break;
	      case UNORDERED:
		aSE = [anOrch allocUnitGenerator:el->class]; 
		break;
	      case PATCHPOINT:
		aSE = [anOrch allocPatchpoint:el->seg];
		break;
	      case SYNTHDATA:
		aSE = [anOrch allocSynthData:el->seg length:el->length];
		break;
	    }
	    if (aSE)
	      [aPatch _add:aSE];
	    else {
		[aPatch _free];
		[anOrch endAtomicSection];
		return nil;
	    }
	}    
    }
    if (![aPatch _connectContents]) {
	[aPatch _free];
	[anOrch endAtomicSection];
	return nil;
    }
    if (_MK_ORCHTRACE(anOrch,MK_TRACEORCHALLOC))
      _MKOrchTrace(anOrch,MK_TRACEORCHALLOC,
		   "allocSynthPatch connectsContents of %s_%d",
		   [synthPatchClass name],aPatch);
    [anOrch endAtomicSection];
    return aPatch;
}

@end

