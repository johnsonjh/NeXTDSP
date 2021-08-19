#import <objc/List.h>

@interface _MKNameTable : List
{
    int		hashSize;
    id backHash;
}

+ new; 
+ newHashSize:(int )theSize; 
- free; 

@end


















