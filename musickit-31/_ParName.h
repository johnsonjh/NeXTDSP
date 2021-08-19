#import <objc/Object.h>

#import "_parameter.h"

@interface _ParName : Object
{
    BOOL (*printfunc)(_MKParameter *param,NXStream *aStream,
		      _MKScoreOutStruct *p);
    int par;
    char * s;
}

@end



















