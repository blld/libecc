//
//  oplist.h
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#ifndef io_libecc_oplist_h
#ifdef Implementation
#undef Implementation
#include __FILE__
#include "implementation.h"
#else
#include "interface.h"
#define io_libecc_oplist_h

	#include "op.h"

#endif


Interface(OpList,
	
	(struct OpList *, create ,(const Native(Function) native, struct Value value, struct Text text))
	(void, destroy ,(struct OpList *))
	
	(struct OpList *, join ,(struct OpList *, struct OpList *))
	(struct OpList *, join3 ,(struct OpList *, struct OpList *, struct OpList *))
	(struct OpList *, joinDiscarded ,(struct OpList *, uint16_t n, struct OpList *))
	(struct OpList *, unshift ,(struct Op op, struct OpList *))
	(struct OpList *, unshiftJoin ,(struct Op op, struct OpList *, struct OpList *))
	(struct OpList *, unshiftJoin3 ,(struct Op op, struct OpList *, struct OpList *, struct OpList *))
	(struct OpList *, shift ,(struct OpList *))
	(struct OpList *, append ,(struct OpList *, struct Op op))
	(struct OpList *, appendNoop ,(struct OpList *))
	(struct OpList *, createLoop ,(struct OpList * initial, struct OpList * condition, struct OpList * step, struct OpList * body, int reverseCondition))
	
	(void, optimizeWithEnvironment, (struct OpList *, struct Object *environment, uint32_t index))
	
	(void, dumpTo ,(struct OpList *, FILE *file))
	(struct Text, text ,(struct OpList *oplist))
	,
	{
		uint32_t opCount;
		struct Op *ops;
	}
)

#endif
