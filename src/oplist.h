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

#include "interface.h"


Interface(OpList,
	
	(struct OpList *, create ,(const Native(Function) native, struct Value value, struct Text text))
	(void, destroy ,(struct OpList *))
	
	(struct OpList *, join ,(struct OpList *, struct OpList *))
	(struct OpList *, joinDiscarded ,(struct OpList *, uint16_t n, struct OpList *))
	(struct OpList *, unshift ,(struct Op op, struct OpList *))
	(struct OpList *, shift ,(struct OpList *))
	(struct OpList *, append ,(struct OpList *, struct Op op))
	(struct OpList *, appendNoop ,(struct OpList *))
	(struct OpList *, createLoop ,(struct OpList * initial, struct OpList * condition, struct OpList * step, struct OpList * body, int reverseCondition))
	
	(void, optimizeWithContext, (struct OpList *, struct Object *context))
	
	(void, dumpTo ,(struct OpList *, FILE *file))
	(struct Text, text ,(struct OpList *oplist))
	,
	{
		uint32_t opCount;
		struct Op ops[1];
	}
)

#endif
