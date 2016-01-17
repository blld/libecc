//
//  namespace_io_libecc.h
//  libecc
//
//  Created by Bouilland Aurélien on 25/07/2015.
//  Copyright (c) 2015 Libeccio. All rights reserved.
//

#ifndef io_libecc_namespace_h
#define io_libecc_namespace_h

#include "compatibility.h"

#define Ecc         io_libecc_Ecc
#define             io_libecc_Ecc(X) \
                    io_libecc_ecc_## X

#define Input       io_libecc_Input
#define             io_libecc_Input(X) \
                    io_libecc_input_## X

#define Lexer       io_libecc_Lexer
#define             io_libecc_Lexer(X) \
                    io_libecc_lexer_## X

#define Parser      io_libecc_Parser
#define             io_libecc_Parser(X) \
                    io_libecc_parser_## X

#define Value       io_libecc_Value
#define             io_libecc_Value(X) \
                    io_libecc_value_## X

#define Op          io_libecc_Op
#define             io_libecc_Op(X) \
                    io_libecc_op_## X

#define OpList      io_libecc_OpList
#define             io_libecc_OpList(X) \
                    io_libecc_oplist_## X

#define Date        io_libecc_Date
#define             io_libecc_Date(X) \
                    io_libecc_date_## X

#define Key         io_libecc_Key
#define             io_libecc_Key(X) \
                    io_libecc_key_## X

#define Native      io_libecc_Native
#define             io_libecc_Native(X) \
                    io_libecc_native_## X

#define Pool        io_libecc_Pool
#define             io_libecc_Pool(X) \
                    io_libecc_pool_## X

#define Env         io_libecc_Env
#define             io_libecc_Env(X) \
                    io_libecc_env_## X

#define Text        io_libecc_Text
#define             io_libecc_Text(X) \
                    io_libecc_text_## X

#define Chars       io_libecc_Chars
#define             io_libecc_Chars(X) \
                    io_libecc_chars_## X

#define Entry       io_libecc_Entry
#define             io_libecc_Entry(X) \
                    io_libecc_entry_## X

#define Object      io_libecc_Object
#define             io_libecc_Object(X) \
                    io_libecc_object_## X

#define Array       io_libecc_Array
#define             io_libecc_Array(X) \
                    io_libecc_array_## X

#define Arguments   io_libecc_Arguments
#define             io_libecc_Arguments(X) \
                    io_libecc_arguments_## X

#define Error       io_libecc_Error
#define             io_libecc_Error(X) \
                    io_libecc_error_## X

#define String      io_libecc_String
#define             io_libecc_String(X) \
                    io_libecc_string_## X

#define Function    io_libecc_Function
#define             io_libecc_Function(X) \
                    io_libecc_function_## X

#define Math        io_libecc_Math
#define             io_libecc_Math(X) \
                    io_libecc_math_## X

#define Number      io_libecc_Number
#define             io_libecc_Number(X) \
                    io_libecc_number_## X

#define Boolean     io_libecc_Boolean
#define             io_libecc_Boolean(X) \
                    io_libecc_boolean_## X

#define RegExp      io_libecc_RegExp
#define             io_libecc_RegExp(X) \
                    io_libecc_regexp_## X

#endif
