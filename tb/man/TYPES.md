# Types

The TBIR data types are used to represent the structure and certain proofs
about the data itself.

## Void (TB_VOID)

void type is a unit type and thus cannot hold data.

## Boolean (TB_BOOL)

booleans represent either true or false and the conversion is defined as:

`((x != 0) ? true : false) where x is a data-holding type`

this is important to note because in float types NaN comparisons always return
false which means that NaN is considered false in a (NaN -> bool) conversion.

## Integers (TB_I8, TB_I16, TB_I32, TB_I64)

integer types come in a few basic sizes (i8, i16, i32, i64) and represent numerical
data and raw data. Integer operations can come in two forms: signed and unsigned.

## Floats (TB_F32, TB_F64)

floating point types are IEEE-754-2008 compliant with f32, and f64 mapping to binary32,
and binary64 respectively.

## Pointers (TB_PTR)

Pointers refer to memory objects in the global address space (see MEMORY.md)
TODO: currently TB only supports on address space but this might be subject to change
