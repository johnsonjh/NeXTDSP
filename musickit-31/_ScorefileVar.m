#ifdef SHLIB
#include "shlib.h"
#endif

/*
  ScorefileVar.m
  Copyright 1987, NeXT, Inc.
  Responsibility: David A. Jaffe
  
  DEFINED IN: The Music Kit
  HEADER FILES: musickit.h
*/
/* 
Modification history:

  09/18/89/daj - Changed myParameter to be a _MKParameter * instead of an object.
  10/06/89/daj - Changed to use hashtable.h version of table.
  03/21/90/daj - Small changes to quiet -W compiler warnings.

*/

#import "_musickit.h"
#import <ctype.h>

#import "_ParName.h"
#import "_ScorefileVar.h"
@implementation _ScorefileVar: Object
/* This class is used for variable values. Setting a ScorefileVar never
   changes its type unless it is an Untyped score var. Automatic type
   conversion is done where possible. 

   This is a private musickit class.
       */
{
    _MKToken token;
    _MKParameter *myParameter;          /* Used internally to store value. */
    BOOL (*preDaemon)();   
    /* preDaemon is an optional function of three arguments: 
       id varObject; _MKToken newValueType; and char *ptrToNewValue;
       It is called before the value is set and is used to filter bad values.
       It returns YES if the value should be set or NO if it should not be set.
       */
    void (*postDaemon)();
    /* postDaemon is an optional function of one arguments:
       id ScorefileVarObject;
       It is called after the value has been set. 
       */
    BOOL readOnly;   /* YES, if variable should not be changed. */
    char * s;
}

id _MKNewScorefileVar(_MKParameter *parObj,char * name,BOOL untyped,BOOL isReadOnly)
    /* You supply the parObj yourself. The object is not copied.*/
{	
    _ScorefileVar *self = [_ScorefileVar new];
    self->s = _MKMakeStr(name);
    self->token = (untyped) ? _MK_untypedVar : _MK_typedVar;
    self->readOnly = isReadOnly;
    self->preDaemon = NULL;
    self->postDaemon = NULL;
    self->myParameter = parObj;
    return self;
}

id _MKSetReadOnlySFVar(_ScorefileVar *self,BOOL yesOrNo)
{
    self->readOnly = yesOrNo; 
    return self;
}

int _MKSetDoubleSFVar(_ScorefileVar *self,double floval)
    /* Sets receiver, doing appropriate type conversion to type
       of receiver. Attempting to set an envelope-valued typed ScorefileVar 
       with this method generates an error. */
{
    if (self->readOnly)
      return (int)MK_sfReadOnlyErr;
    if (self->preDaemon) {
	_MKParameterUnion aParameterNode;        
	aParameterNode.rval = floval;
	if (!self->preDaemon(self,MK_double,&aParameterNode)) 
	  return 0;
    }
    if (self->token == _MK_untypedVar) 
      _MKSetDoublePar(self->myParameter,floval);
    else switch (self->myParameter->_uType) {
      case MK_double: 
	_MKSetDoublePar(self->myParameter,floval);
	break;
      case MK_int:
	_MKSetIntPar(self->myParameter,(int)floval);
	break;
      case MK_string:
	_MKSetStringPar(self->myParameter,_MKDoubleToStringNoCopy(floval));
	break;
      case MK_waveTable:
      case MK_envelope:
	return (int)MK_sfTypeConversionErr;
    }
    if (self->postDaemon)
      self->postDaemon(self);
    return 0;
}

int _MKSetIntSFVar(_ScorefileVar *self,int  intval)
    /* Sets receiver, doing appropriate type conversion to type
       of receiver. Attempting to set an envelope-valued typed ScorefileVar 
       with this method generates an error. */
{
    if (self->readOnly)
      return (int)MK_sfReadOnlyErr;
    if (self->preDaemon) {
	_MKParameterUnion aParameterNode;        
	aParameterNode.ival = intval;
	if (!self->preDaemon(self,MK_int,&aParameterNode)) 
	  return 0;
    }
    if (self->token == _MK_untypedVar) {
	_MKSetIntPar(self->myParameter,intval);
    }
    else switch (self->myParameter->_uType) {
      case MK_double: 
	_MKSetDoublePar(self->myParameter,(double)intval);
	break;
      case MK_int:
	_MKSetIntPar(self->myParameter,intval);
	break;
      case MK_string:
	_MKSetStringPar(self->myParameter,_MKIntToStringNoCopy(intval));
	break;
      default:
	return (int)MK_sfTypeConversionErr;
    }
    if (self->postDaemon)
      self->postDaemon(self);
    return 0;
}

int _MKSetStringSFVar(_ScorefileVar *self,char * strval)
    /* Sets receiver, doing appropriate type conversion to type
       of receiver. Attempting to set an envelope-valued typed ScorefileVar 
       with this method generates an error. */
{
    if (self->readOnly)
      return (int)MK_sfReadOnlyErr;
    if (self->preDaemon) {
	_MKParameterUnion aParameterNode;        
	aParameterNode.sval = strval;
	if (!self->preDaemon(self,MK_string,&aParameterNode)) 
	  return 0;
    }
    if (self->token == _MK_untypedVar) {
	_MKSetStringPar(self->myParameter,strval);
    }
    else switch (self->myParameter->_uType) {
      case MK_string:
	_MKSetStringPar(self->myParameter,strval);
	break;
      case MK_double:
	_MKSetDoublePar(self->myParameter,_MKStringToDouble(strval));
	break;
      case MK_int:
	_MKSetIntPar(self->myParameter,_MKStringToInt(strval));
	break;
      default:
	return (int)MK_sfTypeConversionErr;
    }
    if (self->postDaemon)
      self->postDaemon(self);
    return 0;
}


static int setToSymbol(_ScorefileVar *self,id sym,int token)
{
    int type;
    if (self->readOnly)
      return (int)MK_sfReadOnlyErr;
    if (self->preDaemon) {
	_MKParameterUnion aParameterNode;        
	aParameterNode.symbol = sym;
	if (!self->preDaemon(self,token,&aParameterNode)) 
	  return 0;
    }
    if (self->token == _MK_untypedVar) 
      _MKSetObjPar(self->myParameter,sym,token);
    else if ((type = self->myParameter->_uType) == token)
      _MKSetObjPar(self->myParameter,sym,token);
    else return (int)MK_sfTypeConversionErr;
    if (self->postDaemon)
      self->postDaemon(self);
    return 0;
}

int _MKSetEnvSFVar(_ScorefileVar *self,id envelope)
    /* Receiver must be untyped or envelope-typed. Envelope is not copied. */
{
    return setToSymbol(self,envelope,MK_envelope);
}

int _MKSetWaveSFVar(_ScorefileVar *self,id waveTable)
    /* Receiver must be untyped or waveTable-typed. WaveTable is not copied. */
{
    return setToSymbol(self,waveTable,MK_waveTable);
}

int _MKSetObjSFVar(_ScorefileVar *self,id object)
    /* Receiver must be untyped or object-typed. Object is not copied. */
{
    return setToSymbol(self,object,MK_object);
}

int _MKSFVarInternalType(_ScorefileVar *self)
    /* Return type of datum as represented in ScorefileVar. */
{ 
    return self->myParameter->_uType;
}

_MKParameterUnion *_MKSFVarRaw(_ScorefileVar *self)
/* Returns the raw _MKParameterUnion * from the contained Parameter object. */
{
    return _MKParRaw(self->myParameter);
}

_MKParameter *_MKSFVarGetParameter(_ScorefileVar *self)
  /* Private method that returns the Parameter object used to hold the 
     receiver's value. */
{
    return self->myParameter;
}

id _MKSetScorefileVarPreDaemon(self,funPtr)
    _ScorefileVar *self;
    BOOL (*funPtr)();
    /* Assign the function pointed to by funPtr to be used before
       the receiver's value is set. funPtr is a function of three arguments: 
       id ScorefileVarObject; 
       int newValueType; 
       _MKParameterUnion * ptrToNewValue; 
       It is called before the value is set and is used to filter bad values.
       It returns YES if the value should be set or NO if it should not be set.
       This method return self.
       */
{
    self->preDaemon = funPtr;
    return self;
}

id _MKSetScorefileVarPostDaemon(self,funPtr)
    _ScorefileVar *self;
    void (*funPtr)();
    /* Assign the function pointed to by funPtr to be used after
       the receiver's value is set. funPtr is a function of one argument:
       id ScorefileVarObject;  It is called after the value has been set and returns
       no value.        This method return self.
       */
{
    self->postDaemon = funPtr;
    return self;
}

-copy 
    /* Returns a copy of the ScorefileVar with the string datum
       copied. */
{
    _ScorefileVar *rtnVal;
    rtnVal = [super copy];
    rtnVal->myParameter = _MKCopyParameter(myParameter);
    return rtnVal;
}

-writeScorefileStream:(NXStream *)aStream
    /* Writes <ScorefileVarName> = <value>. */
{	
    NXPrintf(aStream,"%s = ",s);
    _MKParWriteValueOn(myParameter,aStream,NULL);
    return self;
}

-free
    /* Frees object */
{
    if ((myParameter->parNum) != MK_privatePars) 
      _MKFreeParameter(myParameter); /* Otherwise it's a shared parval. Don't touch it in 
					that case. */
    NX_FREE(s);
    return [super free];
}

-(char *)name
{
    return s;
}

@end

