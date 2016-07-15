/**
 * @file type.c  Ordered Dictionary -- value types
 *
 * Copyright (C) 2010 - 2015 Creytiv.com
 */

#include "re_types.h"
#include "re_fmt.h"
#include "re_mem.h"
#include "re_list.h"
#include "re_hash.h"
#include "re_odict.h"


bool odict_type_iscontainer(enum odict_type type)
{
	switch (type) {

	case ODICT_OBJECT:
	case ODICT_ARRAY:
		return true;

	default:
		return false;
	}
}


bool odict_type_isreal(enum odict_type type)
{
	switch (type) {

	case ODICT_STRING:
	case ODICT_INT:
	case ODICT_DOUBLE:
	case ODICT_BOOL:
	case ODICT_NULL:
		return true;

	default:
		return false;
	}
}


const char *odict_type_name(enum odict_type type)
{
	switch (type) {

	case ODICT_OBJECT: return "Object";
	case ODICT_ARRAY:  return "Array";
	case ODICT_STRING: return "String";
	case ODICT_INT:    return "Integer";
	case ODICT_DOUBLE: return "Double";
	case ODICT_BOOL:   return "Boolean";
	case ODICT_NULL:   return "Null";
	default:           return "???";
	}
}
