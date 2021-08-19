/* 
Modification history:

  09/22/89/daj - Moved _MKNameTable functions to _MKNameTable.h. Added global
                 defines of bits.
  10/06/89/daj - Changed to use hashtable.h version of table.
  01/31/90/daj - Added import of hashtable.h
*/

#import <objc/hashtable.h>

typedef struct __MKBiHash {
    NXHashTable *hTab,*bTab;
} _MKBiHash;

#define _MK_NOFREESTRINGBIT 0x8000 /* Set if string is never to be freed */
#define _MK_AUTOIMPORTBIT 0x4000 /* Set if all LOCAL tables should import */
#define _MK_BACKHASHBIT 0x2000  /* Set if object-to-name lookup is required */

extern _MKBiHash *_MKNewBiHash(int hSize,int bSize);
extern _MKBiHash *_MKFreeBiHash(_MKBiHash *table);

/* Private name table functions */
extern void _MKNameGlobal(char * name,id dataObj,unsigned short type,
			  BOOL autoImport,BOOL copyIt);
extern id _MKGetNamedGlobal(char * name,unsigned short *type);
extern const char *_MKGetGlobalName(id object);

/* Very private name table functions */
extern char *_MKUniqueName(char *name,_MKBiHash *table,id anObject,id *hashObj);
extern _MKBiHash *_MKNewScorefileParseTable(void);
extern id _MKGetListElementWithName(id aList,char *aName);
extern _MKBiHash *_MKNameTableAddName(_MKBiHash *table,char *theName,
				      id owner, id object,
				      unsigned short type,BOOL copyIt);
extern id _MKNameTableGetFirstObjectForName(_MKBiHash *table,char *theName);
extern id _MKNameTableGetObjectForName(_MKBiHash *table,char *theName,id theOwner,
				unsigned short *typeP);
extern char *_MKNameTableGetObjectName(_MKBiHash *table,id theObject,id *theOwner);
extern _MKBiHash *_MKNameTableRemoveName(_MKBiHash *table,char *theName,id theOwner);
extern _MKBiHash *_MKNameTableRemoveObject(_MKBiHash *table,id theObject);

#define _MKFreeScorefileTable(_table) _MKFreeBiHash(_table) /* This should go away */

extern char *_MKSymbolize(char *sym,BOOL *wasChanged);
















