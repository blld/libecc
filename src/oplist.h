//
//  oplist.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_oplist_h
#define io_libecc_oplist_h

#include "namespace_io_libecc.h"

#include "op.h"


#define io_libecc_interface_noIdentity
#include "interface.h"

#define Module \
	io_libecc_OpList

Interface(
	(Instance, create ,(const Native native, struct Value value, struct Text text))
	(void, destroy ,(Instance))
	
	(Instance, join ,(Instance, Instance))
	(Instance, unshift ,(struct Op op, Instance))
	(Instance, append ,(Instance, struct Op op))
	(Instance, appendNoop ,(Instance))
	(Instance, createLoop ,(Instance initial, Instance condition, Instance step, Instance body, int reverseCondition))
	
	(void, optimizeWithContext, (Instance, struct Object *context))
	
	(void, dumpTo ,(Instance, FILE *file))
	(struct Text, text ,(struct OpList *oplist))
	,
	{
		uint32_t opCount;
		struct Op ops[];
	}
)

#endif
