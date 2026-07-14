# Places and storage locations

"Places" as represented by the `lir::place` type and "storage locations" as
represented by the `storage_loc` type are concepts used by the LIR datastructure
and the lowering algorithm. Some of the underlying ideas are inspired by the
Rust MLIR representation while others were added based on personal ideas.

## Places

A "place" is used to describe something from which we want to get a value or to
which we want to store a value. However at this point, this "something" should
be understood as a more high-level or abstract conceps rather than actual
storage locations, like a memory address or a CPU register. Instead a "place" is
used to combine a variable of the language with several "modifiers" for how this
variable is used or accessed. Modifiers can be such things as pointer
dereferences, field accesses or maybe array indices.

To make an example, when we write `my_record.field1 = 5` in our source code, on
the surface this looks like we are using the variable `my_record`. On closer
inspection however, it becomes clear, that we actually only care about the field
`field1` of this record-typed variable. Another example would be `*my_ptr = 37`
where again on the surface it looks like we are writing to variable `my_ptr`.
But the `*` in front of the variable name makes it clear, that we want to use
the value of this variable as a pointer into memory which is in turn where we
are going to write to. So the "place" here is a memory location that we get from
dereferencing the pointer stored in the variable `my_ptr`.

Such places can be rather complex like `*record.f1.value.ptr` which involves
multiple field accesses and a pointer dereferencing.

In order to handle such places of different complexities in a more uniform way,
capylang extracts `place` instances from AST sub-trees of pointer dereferencing,
field access and variable accesses. Each `place` contains two main elements

 * A `base`, which represents the initial variable from which the full "place"
   is determined. This field consists of a variable name that can be used for
   quick access and a symbol reference which can be used to get all other
   details about the variable.
 * A `projection`, which is basically an ordered list of what we call "place
   elements". The place elements represent what was described as "modifiers" in
   the above examples.

At the moment only the following place elements are implemented

 * A `deref` represents a pointer dereferencing. It has no additional members.
 * A `field` represents a struct field access. This place element contains the
   index of the field to be accessed.

## Storage locations

While places are used to handle something similar to storage locations but on a higher abstraction level, the concept of "storage locations" described in this
section is very closely related to WASM-level of the implementation. On this 
level we distinguish actual WASM-VM concepts such as

 * local variable; `sl_local_var`
 * function argument, `sl_argument`
 * global variable, `sl_global_var`
 * a memory location in the linear memory, `sl_ptr`

All variables and the arguments contain the field `name` containing the variable
name and the field `type` containing the type id of this variable.

The memory location type `sl_ptr` contains a memory `offset` value and a field
`type` which contains the type id of the value stored at the address in linear
memory to which this pointer points to.

It is important to understand, that the full information of `sl_ptr` is a
combination of what is stored in that struct plus what is currently on the WASM
parameter stack. Any transformation step of the conversion algorithm which
produces an `sl_ptr` as a result also has to generate WASM instructions that
put a pointer value on the WASM parameter stack.

## Conversion algorithm

After all operations and lowerings in LIR are completed, the LIR nodes are
converted (in this project "emitted") into WASM instructions. This happens 
through a sequence of transformations using the place elements one after the
next.

 1. As a starting point, a `place` instance is turned into a `storage_loc`
    instance by calling `transform_symbol()`. During this transformation no code
    is generated as we do not yet want to deal with the details of whether it
    will be a load or store operation later on.
    The first `storage_loc` is derived from the variable symbol stored in `base`
    and can only return a local or global variable or a function argument.
 2. The following transformation steps are handled through `transform_place()`
    which takes a previous `storage_loc` and the next `place_elem` in the
    `.projection` list of the place that is currently transformed, and turns
    this into a new `storage_loc` instance.
    During these transformations, WASM instructions might be emitted to produce
    any necessary transformations in the WASM world.
 3. When all place elements are transformed, the final step will be handled
    depending on whether we are in a load or in a store context. So we will
    either generate the necessary load or get instruction or instead the
    necessary store or set instruction.

## Next steps

Currently this places-and-storage-location concept works very well for record
field access and pointer dereferencings with what is used in example.capy and in
the test inputs. However there are still a few open points that need to be
checked, extended or even be turned into a concept and implementation in the
future. Among these things are

 * Records as values; so far only some cases of records being treated as values
   are proven to work, mainly when using them as strings. However other cases
   are not properly tested yet and may still fail to work as expected.
 * Arrays; any kind of index-based access is so far completely missing. It will
   definitly need a new type of place element but will also definitly need a lot
   of additional work in the whole system, including AST and LIR definitions.
