//
//  date.c
//  libecc
//
//  Copyright (c) 2019 Aurélien Bouilland
//  Licensed under MIT license, see LICENSE.txt file in project root
//

#include "date.h"

// MARK: - Private

static struct Object *datePrototype = NULL;
static struct Object *dateConstructor = NULL;

const struct Object(Type) Date(type) = {
	.text = &Text(dateType),
};

// MARK: - Static Members

// MARK: - Methods

void setup (void)
{
	datePrototype = Object.create(Object(prototype));
	datePrototype->type = &Date(type);
	
//	Function.addToObject(arrayPrototype, "toString", toString, 0);
//	Function.addToObject(arrayPrototype, "toLocaleString", toString, 0);
//	Function.addToObject(arrayPrototype, "valueOf", valueOf, 0);
//	Function.addToObject(arrayPrototype, "hasOwnProperty", hasOwnProperty, 0);
//	Function.addToObject(arrayPrototype, "isPrototypeOf", isPrototypeOf, 0);
//	Function.addToObject(arrayPrototype, "propertyIsEnumerable", propertyIsEnumerable, 0);
}

void teardown (void)
{
//	Object.destroy(datePrototype), datePrototype = NULL;
}

struct Object *prototype (void)
{
	return datePrototype;
}

//struct Object *constructor (void)
//{
//	return dateConstructor;
//}
