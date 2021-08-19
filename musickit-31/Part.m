#ifdef SHLIB
#include "shlib.h"
#endif

/*
  Part.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  01/24/90/daj - Fixed bug in removeNote:. 
  03/19/90/daj - Added MKSetPartClass() and MKGetPartClass().
  03/21/90/daj - Added archiving support.
  04/21/90/daj - Small mods to get rid of -W compiler warnings.
  04/28/90/daj - Flushed scoreClass optimization, now that we're a shlib.
  08/14/90/daj - Fixed bug in addNote:. It wasn't checking to make sure notes exists.
  08/23/90/daj - Changed to zone API.
  10/04/90/daj - freeNotes now creates a new notes List.
*/

#import "_musickit.h"  
#import <objc/HashTable.h>
#import "_Score.h"
#import "_Note.h"
#import "_Part.h"


@implementation Part:Object
/* A Part is a time-ordered collection of Notes that can be edited,
 * performed, and realized.
 *
 * One or more Parts can be grouped together in a Score.  
 *
 * Editing a Part refers generally to adding and removing Notes, 
 * not to changing
 * the contents of the Notes themselves (although some methods
 * do both; see \fBsplitNotes\fR and \fBcombineNotes\fR).
 * Notes are ordered within the Part by their timeTag values.  
 * To move a Note within
 * a Part, you simply change its timeTag by sending it the appropriate
 * message (see the Note class).  This effectively removes the Note
 * from its Part, changes the timeTag, and then adds it back to its Part.
 * 
 * A Part can be performed using a PartPerformer and can 'record' notes
 * by using a PartRecorder. You must not free a Part or any of the Notes 
 * in a Part while there are any PartPerformers using the Part. It is ok
 * to record to a part and perform that part at the same time because the 
 * PartPerformer takes a snap-shot of the Part when the PartPerformer 
 * is activated.
 *
 * The Notes in a Part are stored in a List object. The List is only sorted
 * when necessary. In particular, the List is sorted, if necessary, when an 
 * access method is invoked. The access methods are:
 * 
 * * - firstTimeTag:(double)firstTimeTag lastTimeTag:(double)lastTimeTag;
 * * - atTime:(double )timeTag; 
 * * - atOrAfterTime:(double )timeTag; 
 * * - nth:(unsigned )n; 
 * * - atOrAfterTime:(double )timeTag nth:(unsigned )n; 
 * * - atTime:(double )timeTag nth:(unsigned )n; 
 * * - next:aNote; 
 * * - notes;
 * 
 * Other methods that cause a sort, if necessary, are:
 * 
 * * - combineNotes;
 * * - removeNotes:aList;
 * * - removeNote:aNote; 
 * 
 * Methods that may alter the List such that its Notes are no longer sorted are
 * the following:
 * 
 * * - addNoteCopies:aList timeShift:(double )shift; 
 * * - addNotes:aList timeShift:(double )shift; 
 * * - addNote:aNote; 
 * * - addNoteCopy:aNote; 
 * * - splitNotes
 * 
 * This scheme works well for most cases. However, there are situations where
 * it can be problematic. For example:
 *
 * * for (i=0; i<100; i++) {
 * * [aPart addNote:anArray[i]];
 * * [aPart removeNote:anotherArray[i]];
 * * }
 *
 * In this case, the Part will be sorted each time removeNote: is called, 
 * causing N-squared behavior. You can get around this by first adding all the 
 * notes using addNotes: and then removing all the notes using removeNotes:.  
 * 
 * In some cases, you may find it most convenient to 
 * remove the Notes from the Part, modify them in your own 
 * data structure, and then reinsert them into the Part. 
 * 
 * You can explicitly trigger a sort (if needed) by sending the -sort message.
 * This is useful if you ever subclass Part.
 * 
 * To get a sorted copy of the List of notes use the -notes method.
 * To get the List of notes itself, use the -notesNoCopy method. 
 * -notesNoCopy does not guarantee the Notes are sorted.
 * If you want to examine the Notes in-place sorted, first send -sort, then
 * -notesNoCopy.
 * 
 * You can find out if the List is currently sorted by the -isSorted method.
 * 
 */
{
    id score;              /* The score to which this Part belongs. */
    id notes;       /* List of Notes. */
    id info;        /* A Note used to store an arbitrary collection of info
                      associated with the Part. */
    int noteCount;  /* Number of Notes in the Part. */
    BOOL isSorted;  /* YES if the receiver is sorted. */
    id _reservedPart1;
    id _reservedPart2;
    int _reservedPart3;
}
#define _aNoteSender _reservedPart1 /* Used only by ScorefilePerformers. */
#define _activePerformanceObjs _reservedPart2
#define _highestOrderTag _reservedPart3 /* For disambiguating binary search
					   on identical time tagged Notes. */

/* METHOD TYPES
 * Creating and freeing a Part
 * Querying the object
 * Modifying the object
 * Reading and writing scorefiles
 * Realization
 * Editing the Part
 */

#define VERSION2 2

+initialize
{
    if (self != [Part class])
      return self;
    [self setVersion:VERSION2];
    return self;
}

+new
  /* Create a new instance and sends [self init]. */
{
    self = [self allocFromZone:NXDefaultMallocZone()];
    [self init];
    [self initialize]; /* Avoid breaking pre-2.0 apps */
    return self;
}

static id theSubclass = nil;

BOOL MKSetPartClass(id aClass)
{
    if (!_MKInheritsFrom(aClass,[Part class]))
      return NO;
    theSubclass = aClass;
    return YES;
}

id MKGetPartClass(void)
{
    if (!theSubclass)
      theSubclass = [Part class];
    return theSubclass;
}

/* Format conversion methods. ---------------------------------------------*/


static id compact(Part *self)
{
    id *el,*endEl;
    id newList = [List newCount:self->noteCount];
    IMP addObjectImp;
#   define ADDNEW(x) (*addObjectImp)(newList, @selector(addObject:),x)
    addObjectImp = [newList methodFor:@selector(addObject:)];
    el = NX_ADDRESS(self->notes); 
    endEl = el + self->noteCount; 
    while (el < endEl) {
        if (_MKNoteIsPlaceHolder(*el)) {
	    [*el++ _setPartLink:nil order:0];
            self->noteCount--; 
        }
        else ADDNEW(*el++);
    }
    [self->notes free];
    self->notes = newList;
    return self;
}

static void removeNote(Part *self,id aNote);

-combineNotes
  /* TYPE: Editing 
   * Attempts to minimize the number of Notes by creating
   * a single noteDur for every noteOn/noteOff pair
   * (see the Note class).  A noteOn is paired with the 
   * earliest subsequent noteOff that has a matching noteTag. However,
   * if an intervening noteOn or noteDur is found, the noteOn is not 
   * converted to a noteDur.
   * If a match isn't found, the Note is unaffected.
   * Returns the receiver.
   */
{
    id aList,noteOn,aNote;
    int noteTag,listSize;
    register int i,j;
    if (!noteCount)
      return self;
    aList = [self notes];
    listSize = noteCount;
#   define REMOVEAT(x) *(NX_ADDRESS(aList) + x) = nil
#   define AT(x) NX_ADDRESS(aList)[x]
    for (i = 0; i < listSize; i++)             /* For each note... */
      if ([noteOn = AT(i) noteType] == MK_noteOn) {
          noteTag = [noteOn noteTag];         /* We got a noteOn */
          if (noteTag == MAXINT)              /* Malformed Part. */
            continue;
          for (j = i + 1; (j < listSize); j++)      /* Search forward */
            if ((aNote = AT(j)) &&              /* Watch out for nils */
                ([aNote noteTag] == noteTag)) { /* A hit ? */
                switch ([aNote noteType]) {    /* Ok. What is it? */
                  case MK_noteOff:
                    removeNote(self,aNote);    /* Remove aNote from us */
                    [noteOn setDur:([aNote timeTag] - [noteOn timeTag])];
                    [noteOn _unionWith:aNote]; /* Ah... love. */
                    REMOVEAT(j);               /* Remove from aList */
                    [aNote free];              /* No break; here */
                  case MK_noteOn:              /* We don't search on     */
                  case MK_noteDur:             /*   if we find on or dur */
                    j = listSize;              /* Force abort. No break; */
                  default:                     
                    break;
                }                           /* End of switch */
            }                               /* End of search forward */
      }                                     /* End of if noteOn */
    [aList free];                           
    compact(self);
    return self;
}

-splitNotes
  /* TYPE: Editing
   * Splits all noteDurs into a noteOn/noteOff pair.
   * This is done by changing the noteType of the noteDur to noteOn
   * and creating a noteOff with timeTag equal to the 
   * timeTag of the original Note plus its duration.
   * However, if an intervening noteOn or noteDur of the same tag
   * appears, the noteOff is not added (the noteDur is still converted
   * to a noteOn in this case).
   * Returns the receiver, or \fBnil\fR if the receiver contains no Notes.
   */
{
    register id aList,noteOff,*el, *el2, *elEnd;
    BOOL abort;
    double timeTag;
    int noteTag;
    unsigned n;
    IMP selfAddNote;
    if (!noteCount)
      return self;
    aList = [self notes];
    elEnd = NX_ADDRESS(aList) + noteCount;
    selfAddNote = [self methodFor:@selector(addNote:)];
#   define SELFADDNOTE(x) (*selfAddNote)(self, @selector(addNote:), (x))
    for (el = NX_ADDRESS(aList), n = noteCount; n--; el++)
      if ([*el noteType] == MK_noteDur) {
          noteOff = [*el _splitNoteDurNoCopy];/* Split all noteDurs. */
          noteTag = [noteOff noteTag];
          if (noteTag == MAXINT)              /* Add noteOff if no tag. */
            SELFADDNOTE(noteOff);
          else {                /* Need to check for intervening Note. */
              abort = NO; 
              timeTag = [noteOff timeTag]; 
              for (el2 = el + 1;        
                   (el2 < elEnd) && ([*el2 timeTag] <= timeTag);
                   el2++)
                if ([*el2 noteTag] == noteTag)
                  switch ([*el2 noteType]) {
                    case MK_noteOn:
                    case MK_noteOff:
                    case MK_noteDur:
                      [noteOff free];
                      el2 = elEnd;          /* Force break of loop. */
                      abort = YES;          /* Forget it. */
                      break;
                    default:
                      break;
                  }
              if (!abort)                   /* No intervening notes. */
                SELFADDNOTE(noteOff);
          }
      }
    [aList free];
#   undef SELFADDNOTE
    return self;
}

/* Reading and Writing files. ------------------------------------ */


-_setInfo:aInfo
  /* Needed by scorefile parser  */
{
    if (!info)
      info = [aInfo copy];
    else 
      [info copyParsFrom:aInfo];
    return self;
}

/* Score Interface. ------------------------------------------------------- */


-addToScore:(id)newScore
  /* TYPE: Modifying
   * Removes the receiver from its present Score, if any, and adds it
   * to \fInewScore\fR.  
   */
{
    return [newScore addPart:self];
}

-removeFromScore
  /* TYPE: Modifying
   * Removes the receiver from its present Score.
   * Returns the receiver, or \fBnil\fR if it isn't part of a Score.
   * (Implemented as \fB[score removePart:self]\fR.)
   */
{
    return [score removePart:self];
}

/* Creation. ------------------------------------------------------------ */

-initialize 
  /* For backwards compatibility */
{ 
    return self;
} 

-init
  /* TYPE: Creating and freeing a Part
   * Initializes the receiver:
   *
   * Sent by the superclass upon creation;
   * You never invoke this method directly.
   * An overriding subclass method should send \fB[super initialize]\fR
   * before setting its own defaults. 
   */
{
    notes = [List new];
    isSorted = YES;
    return self;                        
}

/* Freeing. ------------------------------------------------------------- */

-free
  /* TYPE: Creating and freeing a Part
   * Frees the receiver and its Notes, including the info note, if any.
   * Also removes the name, if any, from the name table.
   * Illegal while the receiver is being performed. In this case, does not
   * free the receiver and returns self. Otherwise returns nil.
   */
{
    if (![self freeNotes])
      return self;
    MKRemoveObjectName(self);
    return [self freeSelfOnly];
}

static void unsetPartLinks(Part *aPart)
{
    Note **el;
    unsigned n;
    if (aPart->notes)
      for (n = [aPart->notes count], el = (Note **)NX_ADDRESS(aPart->notes); 
	   n--;
	   )
	[*el++ _setPartLink:nil order:0];
}

-freeNotes
  /* TYPE: Editing
   * Removes and frees all Notes from the receiver including the info 
   * note, if any.
   * Doesn't free the receiver.
   * Returns the receiver.
   */
{
    if (_activePerformanceObjs)
      return nil;
    info = [info free]; 
    if (notes) {
	unsetPartLinks(self);
	[notes freeObjects];
	[notes free];
	notes = [List new];
	noteCount = 0;
    }
    return self;
}

-freeSelfOnly
  /* TYPE: Creating and freeing a Part
   * Frees the receiver but not their Notes.
   * Returns the receiver. */
{
    [score removePart:self];
    [notes empty];
    notes = [notes free];
    return [super free];
}

/* Compaction and sorting ---------------------------------------- */

#import <stdlib.h>

static id sortIfNeeded(Part *self)
{
    if (!self->isSorted) {
        qsort((void *)NX_ADDRESS(self->notes),(size_t)self->noteCount,
              (size_t)sizeof(id),_MKNoteCompare);
        self->isSorted = YES;
        return self;
    }
    return nil;
}

-(BOOL)isSorted
{
    return isSorted;
}

-sort
  /* If the receiver needs to be sorted, sorts and returns self. Else
     returns nil. */
{
    return sortIfNeeded(self);
}

static id *findNote(Part *self,id aNote)
{       
    return bsearch((void *)&aNote,(void *)NX_ADDRESS(self->notes),
                   (size_t)self->noteCount,(size_t)sizeof(id),_MKNoteCompare);
}    

static id *findAux(Part *self,double timeTag)
{       
    /* This function returns:
       If no elements in list, NULL.
       If the timeTag equals that of the first Note or the timeTag is less 
       than that of the first Note, a pointer to the first Note.
       Otherwise, a pointer to the last Note with timeTag less than the one
       specified. */

    register id *low = NX_ADDRESS(self->notes); 
    register id *high = low + self->noteCount;
    register id *tmp = low + ((unsigned)((high - low) >> 1));
    if (self->noteCount == 0)
      return NULL;
    while (low + 1 < high) {
        tmp = low + ((unsigned)((high - low) >> 1));
        if (timeTag > [*tmp timeTag])
          low = tmp;
        else high = tmp;
    }
    return low;
}

static id *findAtOrAfterTime(Part *self,double firstTimeTag)
{
    id *el = findAux(self,firstTimeTag);
    if (!el)
      return NULL;
    if ([*el timeTag] >= firstTimeTag)
      return el;
    if (++el < NX_ADDRESS(self->notes) + self->noteCount)
      return el;
    return NULL;
}

static id *findAtOrBeforeTime(Part *self,double lastTimeTag)
{
    id *el = findAux(self,lastTimeTag);
    if (!el)
      return NULL;
    if (++el < NX_ADDRESS(self->notes) + self->noteCount)
      if ([*el timeTag] <= lastTimeTag)
        return el;
    el--;
    if (el < NX_ADDRESS(self->notes))
      return NULL;
    if ([*el timeTag] > lastTimeTag)
      return NULL;
    return el;
}

 /* Basic editing operations. ---------------------------------------  */

-addNote:aNote
  /* TYPE: Editing
   * Removes \fIaNote\fR from its present Part, if any, and adds it
   * to the receiver.  aNote must be a Note.
   * Returns the old Part, if any.
   */
{
    id oldPart;
    if (!aNote)
      return nil;
    [oldPart = [aNote part] removeNote:aNote];
    [aNote _setPartLink:self order:++_highestOrderTag];
    if ((noteCount++) && (isSorted)) {
        id lastObj = [notes lastObject];
        if (_MKNoteCompare(&aNote,&lastObj) == -1)
          isSorted = NO;
    }
    [notes addObject:aNote];
    return oldPart;
}

-addNoteCopy:aNote
  /* TYPE: Editing
   * Adds a copy of \fIaNote\fR
   * to the receiver.  
   * Returns the new Note.
   */
{
    id newNote = [aNote copyFromZone:NXDefaultMallocZone()];
    [self addNote:newNote];
    return newNote;
}

static BOOL suspendCompaction = NO;

static void removeNote(Part *self,id aNote)
{
    id *where = findNote(self,aNote);
    if (where)  /* Note in Part? */
      _MKMakePlaceHolder(aNote); /* Mark it as 'to be removed' */
}

-removeNote:aNote
  /* TYPE: Editing
   * Removes \fIaNote\fR from the receiver.
   * Returns the removed Note or \fBnil\fR if not found.
   * You shouldn't free the removed Note if
   * there are any active Performers using the receiver.
   * 
   * Keep in mind that if you have to remove a large number of Notes,
   * it is more efficient to put them in a List and then use removeNotes:.
   */
{
    id *where;
    if (!aNote)
      return nil;
    sortIfNeeded(self);
    if (suspendCompaction) 
      removeNote (self,aNote);
    else {
        where = findNote(self,aNote);
        if (where) {
            noteCount--;
            [notes removeObjectAt:(unsigned)(where - NX_ADDRESS(notes))];
            [aNote _setPartLink:nil order:0]; /* Added Jan 24, 90 */
        }
    }
    return nil;
}

/* Contents editing operations. ----------------------------------------- */


- removeNotes:aList
  /* TYPE: Editing
   * Removes from the receiver all Notes common to the receiver
   * and \fIaList\fR. 
   * Returns the receiver.
   */
{ 
    register id *el;
    unsigned n;
    if (!aList)
      return self;
    for (el = NX_ADDRESS(aList), n = [aList count]; n--; el++)
      removeNote(self,*el);
    compact(self);
    return self;
}

- addNoteCopies:aList timeShift:(double) shift     
  /* TYPE: Editing
   * Copies the Notes in \fIaList\fR, shifts the copies'
   * timeTags by \fIshift\fR beats, and then adds them
   * to the receiver.  \fIaList\fR isn't altered.
   * aList should contain only Notes.
   * Returns the receiver.
   */
{ 
    id *el;
    unsigned n;
    double tTag;
    register id element, copyElement;
    NXZone *zone = NXDefaultMallocZone();
    IMP selfAddNote;
    if (aList == nil)
      return nil;
    selfAddNote= [self methodFor:@selector(addNote:)];
#   define SELFADDNOTE(x) (*selfAddNote)(self, @selector(addNote:), (x))
    for (el = NX_ADDRESS(aList), n = [aList count]; n--; el++)
      {
          element = *el;
          copyElement = [element copyFromZone:zone];
          tTag = [element timeTag];
          if (tTag < (MK_ENDOFTIME-1))
            [copyElement setTimeTag:tTag + shift];
          SELFADDNOTE(copyElement);     
        }
#   undef SELFADDNOTE
    return self;
}

-addNotes:aList timeShift:(double) shift
  /* TYPE: Editing
   * aList should contain only Notes.
   * For each Note in \fIaList\fR, removes the Note
   * from its present Part, if any, shifts its timeTag by
   * \fIshift\fR beats, and adds it to the receiver.
   * 
   * Returns the receiver. 
   */
{ 
    /* In order to optimize the common case of moving notes from one
       Part to another, we do the following.

       First we go through the List, removing the Notes from their Parts,
       with the suspendCompaction set. We also keep track of which Parts we've
       seen. 

       Then we compact each of the Parts.
       
       Finally, we add the Notes. */

    register id *el;
    unsigned n;
    if (aList == nil)
      return self;
    {
        id aPart;
        id parts = [List new];
        IMP addPart = [parts methodFor:@selector(addObjectIfAbsent:)];
#       define ADDPART(x) (*addPart)(parts, @selector(addObjectIfAbsent:), (x))
        suspendCompaction = YES;
        for (el = NX_ADDRESS(aList), n = [aList count]; n--; el++) {
            aPart = [*el part];
            if (aPart) {
                ADDPART(aPart);
                [aPart removeNote:*el]; 
            }
        }
        suspendCompaction = NO;
        for (el = NX_ADDRESS(parts), n = [parts count]; n--; el++) 
          compact(*el);
        [parts free];
    }
    {
        double tTag;
        IMP selfAddNote;
        selfAddNote = [self methodFor:@selector(addNote:)];
#       define SELFADDNOTE(x) (*selfAddNote)(self, @selector(addNote:), (x))
        for (el = NX_ADDRESS(aList), n = [aList count]; n--; el++) {
            tTag = [*el timeTag];
            if (tTag < (MK_ENDOFTIME-1))
              [*el setTimeTag:tTag + shift];
            SELFADDNOTE(*el);
        }
#       undef SELFADDNOTE
    }
    return self;
}

-empty
  /* TYPE: Editing
   * Removes the receiver's Notes but doesn't free them, except for 
   * placeHolder notes, which are freed.
   * Returns the receiver. 
   */
{
    unsetPartLinks(self);
    [notes empty];
    noteCount = 0;
    return self;
}


-shiftTime:(double)shift
  /* TYPE: Editing
   * Shift is added to the timeTags of all notes in the Part. 
   * Implemented in terms of addNotes:timeShift:.
   */
{
    id aList = [self notes];
    id rtn = [self addNotes:aList timeShift:shift];
    [aList free];
    return rtn;
}

/* Accessing ------------------------------------------------------------- */

- firstTimeTag:(double) firstTimeTag lastTimeTag:(double) lastTimeTag 
  /* TYPE: Querying the object
   * Creates and returns a List containing the receiver's Notes
   * between \fIfirstTimeTag\fR and \fIlastTimeTag\fR in time order.
   * The notes are not copied. This method is useful in conjunction with
   * addNotes:timeShift:, removeNotes:, etc.
   */
{
    id aList;
    id *firstEl,*lastEl;
    if (!noteCount)
      return [List new];
    sortIfNeeded(self);
    firstEl = findAtOrAfterTime(self,firstTimeTag);
    lastEl = findAtOrBeforeTime(self,lastTimeTag);
    if (!firstEl || !lastEl || firstEl > lastEl)
      return [List new];
    aList = [List newCount:(unsigned)(lastEl - firstEl) + 1];
    while (firstEl <= lastEl)
      [aList addObject:*firstEl++];
    return aList;
}

-(unsigned)noteCount
  /* TYPE: Querying
   * Return the number of Notes in the receiver.
   */
{
    return noteCount;
}

-(BOOL)containsNote:aNote
  /* TYPE: Querying
   * Returns \fBYES\fR if the receiver contains \fIaNote\fR.
   */
{
    return [aNote part] == self;
}

-(BOOL)isEmpty
  /* TYPE: Querying
   * Returns \fBYES\fR if the receiver contains no Notes.
   */
{
    return (noteCount == 0);
}

- atTime:(double) timeTag
  /* TYPE: Accessing Notes
   * Returns the first Note found at time \fItimeTag\fR, or \fBnil\fR if 
   * no such Note.
   * Doesn't copy the Note.
   */
{
    id *el;
    sortIfNeeded(self);
    el = findAtOrAfterTime(self,timeTag);
    if (!el)
      return nil;
    if ([*el timeTag] != timeTag)
      return nil;
    return *el;
}

-atOrAfterTime:(double)timeTag
  /* TYPE: Accessing Notes
   * Returns the first Note found at or after time \fItimeTag\fR, 
   * or \fBnil\fR if no such Note. 
   * Doesn't copy the Note.
   */
{
    id *el;
    sortIfNeeded(self);
    el = findAtOrAfterTime(self,timeTag);
    return (el) ? *el : nil;
}

- nth:(unsigned) n
  /* TYPE: Accessing Notes
   * Returns the \fIn\fRth Note (0-based), or \fBnil\fR if no such Note.
   * Doesn't copy the Note. */
{
    sortIfNeeded(self);
    return [notes objectAt:n];
}

-atOrAfterTime:(double)timeTag nth:(unsigned) n 
  /* TYPE: Accessing Notes
   * Returns the \fIn\fRth Note (0-based) at or after time \fItimeTag\fR,
   * or \fBnil\fR if no such Note. 
   * Doesn't copy the Note.
   */
{
    id *arrEnd;
    id *el;
    sortIfNeeded(self);
    el = findAtOrAfterTime(self,timeTag);
    if (!el)
      return nil;
    arrEnd = NX_ADDRESS(notes) + noteCount;
    if (n == 0)
      return *el;
    while (n--) {
        if (++el >= arrEnd)
          return nil;
    }
    return *el;
}

-atTime:(double)timeTag nth:(unsigned) n
  /* TYPE: Accessing Notes
   * Returns the \fIn\fRth Note (0-based) at time \fItimeTag\fR,
   * or \fBnil\fR if no such Note. 
   * Doesn't copy the Note.
   */
{
    id aNote = [self atOrAfterTime:timeTag nth:n];
    if (!aNote)
      return nil;
    if ([aNote timeTag] == timeTag)
      return aNote;
    return nil;
}

-next:aNote  
  /* TYPE: Accessing Notes
   * Returns the Note immediately following \fIaNote\fR, or \fBnil\fR
   * if no such Note.  (A more efficient procedure is to create a
   * List with \fBnotes\fR and then step down the List using NX_ADDRESS().
   */
{
    id *el;
    if (!aNote)
      return nil;
    sortIfNeeded(self);
    el = findNote(self,aNote);
    if (!el)
      return nil;
    if (++el == (NX_ADDRESS(notes) + noteCount))
      return nil;
    return *el;
}

/* Querying --------------------------------------------------- */

-copyFromZone:(NXZone *)zone
  /* TYPE: Creating a Part
   * Creates and returns a new Part that contains
   * a copy of the contents of the receiver. The info is copied as well.
   */
{
    Part *rtn = [Part allocFromZone:zone];
    [rtn init];
    [rtn addNoteCopies:notes timeShift:0];
    rtn->info = [info copy];
    return rtn;
}

-copy
{
    return [self copyFromZone:[self zone]];
}

-notesNoCopy
  /* TYPE: Accessing Notes
   * Returns a List of the Notes in the receiver, in time order. 
   * The Notes are not copied. 
   * The List is not copied and is not guaranteed to be sorted.
   */
{
    return notes;
}

-notes
  /* TYPE: Accessing Notes
   * Returns a List of the Notes in the receiver, in time order. 
   * The Notes are not copied. 
   * It is the sender's responsibility to free the List.
   */
{
    sortIfNeeded(self);
    return _MKCopyList(notes);
}

-score  
  /* TYPE: Querying
   * Returns the Score of the receiver's owner.
   */
{
    return score;
}

-info
  /* Returns 'header note', a collection of info associated with each Part,
     which may be used by the App in any way it wants. */ 
{
    return info;
}

-setInfo:aNote
  /* Sets 'header note', a collection of info associated with each Part,
     which may be used by the App in any way it wants. aNote is copied. 
     The old info, if any, is freed. */ 
{
    [info free];
    info = [aNote copy]; 
    return self;
}

- write:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked with NXWriteRootObject(). 
     Archives Notes and info. Also archives Score using 
     NXWriteObjectReference(). */
{
    char *str;
    [super write:aTypedStream];
    sortIfNeeded(self);
    str = (char *)MKGetObjectName(self);
    NXWriteObjectReference(aTypedStream,score);
    NXWriteTypes(aTypedStream,"@@ic*i",&notes,&info,&noteCount,&isSorted,
                 &str,&_highestOrderTag);
    return self;
}

- read:(NXTypedStream *) aTypedStream
  /* You never send this message directly.  
     Should be invoked via NXReadObject(). 
     See write:. */
{
    char *str;
    [super read:aTypedStream];
    if (NXTypedStreamClassVersion(aTypedStream,"Part") == VERSION2) {
        score = NXReadObject(aTypedStream);
        NXReadTypes(aTypedStream,"@@ic*i",&notes,&info,&noteCount,&isSorted,
                    &str,&_highestOrderTag);
        if (str) {
            MKNameObject(str,self);
            NX_FREE(str);
        }
    }
    return self;
}

- awake
  /* Maps noteTags as represented in the archive file onto a set that is
     unused in the current application. This insures that the integrity
     of the noteTag is maintained. */
{
    id tagTable;
    [super awake];
    if ([Score _isUnarchiving])
      return self;
    tagTable = [HashTable newKeyDesc:"i" valueDesc:"i"];
    [self _mapTags:tagTable];
    [tagTable free];
    return self;
}

@end

@implementation Part(Private)

-(void) _mapTags:aHashTable
  /* Must be method to avoid loading Score. AHashTable is a HashTable object
     that maps ints to ints. */
{
    int newTag,oldTag;
    id *el;
    unsigned n;
    sortIfNeeded(self);
    for (el = NX_ADDRESS(notes), n = [notes count]; n--; el++) {
        oldTag = [*el noteTag];
        if (oldTag != MAXINT) { /* Ignore unset tags */
            newTag = (int)[aHashTable valueForKey:(void *)oldTag]; 
            if (!newTag) {
                newTag = MKNoteTag();
                [aHashTable insertKey:(const void *)oldTag value:(void *)newTag];
            }
            [*el setNoteTag:newTag];
        }
    }
}

-(void)_setNoteSender:aNS
  /* Private. Used only by scorefilePerformers. */
{
    _aNoteSender = aNS;
}

-_noteSender
  /* Private. Used only by scorefilePerformers. */
{
    return _aNoteSender;
}

-(void)_unsetScore
    /* Private method. Sets score to nil. */
{
    score = nil;
}
 
-_addPerformanceObj:aPerformer
{
    if (!_activePerformanceObjs)
      _activePerformanceObjs = [List new];
    [_activePerformanceObjs addObjectIfAbsent:aPerformer];
    return self;
}

-_removePerformanceObj:aPerformer
{
     if (!_activePerformanceObjs)
      return nil;
     [_activePerformanceObjs removeObject:aPerformer];
     if ([_activePerformanceObjs count] == 0) {
         [_activePerformanceObjs free];
         _activePerformanceObjs = nil;
     }
    return self;
}

-_setScore:(id)newScore
    /* Removes receiver from the score it is a part of, if any. Does not
       add the receiver to newScore; just sets instance variable. It
       is illegal to remove an active performer. */
{
    id oldScore = score;
    if (score) 
      [score removePart:self];
    score = newScore;
    return oldScore;
}


@end

