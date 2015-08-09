//
//  text.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "text.h"

// MARK: - Private

// MARK: - Static Members

// MARK: - Methods

Structure make (const char *location, uint16_t length)
{
	return (Structure){
		.length = length,
		.location = location,
	};
}

Structure join (Structure from, Structure to)
{
	return make(from.location, to.location - from.location + to.length);
}
