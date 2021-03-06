simple library implementing several useful data structures for C programming

## vec

The vec.h functions are built on top of ordinary C arrays.
So, a vector of type `Type` would be declared as a normal array: `Type *myvec`.
These 'vectors' can interoperate with functions expecting normal arrays, but the reverse is not true.

The `vec_ctor()` macro will magically initialize an array as a vector for you.
Use it like: `vec_ctor(myvec);`

- `myvec` is the null pointer if an allocation fails

to allocate a vector manually, just do: `vec_alloc(&myvec, sizeof *myvec)`.
This function returns `0` on succes, `ENOMEM` if the allocation fails, or EOVERFLOW if you somehow manage to overflow a `size_t` somewhere.

Free a vector with `vec_free(myvec)`

### supported operations:

**important**: every function that modifies its argument takes a pointer to a vector allocated with 
`vec_ctor()` (or `vec_alloc()`) as their first argument.
If you just pass the vector itself, your program might blow up.

- `vec_append(void *vec_ptr, void *item)`

- `vec_insert(void *vec_ptr, void *item, size_t position)`

	- _append_, _prepend_, or _insert_ elements into a vector

	- `item` (or `array`) should be a pointer of the same type as the vector,
	that is: `int *myvec` => `int *item`

	- These functions return `0` on succces, `ENOMEM` if they cannot allocate memory, `EOVERFLOW` if some value would overflow (this should never happen).

	- `vec_insert()` can also return `EINVAL` if the position is out of bounds

- `vec_delete(void *vec_ptr, size_t which)`

	- _delete_ `which` element.

	- Returns nothing, cannot fail.

- `vec_elim(void *vec_ptr, size_t index, size_t nmemb)`

	- _eliminate_ `nmemb` elements from `index` on.

	- Returns nothing, cannot fail.

- `vec_truncate(void *vec_ptr, size_t index)`

	- _truncate_ the vector, starting from `index`.

	- Returns nothing, cannot fail.

- `vec_shift(void *vec_ptr, size_t offset)`

	- _shift_ the vector by `offset` elements.

	- If `offset` is 2, for example, vec[2] becomes vec[0], vec[3] becomes vec[1], and so on

	- Returns nothing, cannot fail.

- `vec_slice(void *vec_ptr, size_t begin, size_t nmemb)`

	- _slice_ `nmemb` elements, starting at `begin`.

	- After `vec_slice(&myvec, 2, 4)`, myvec consists of the four 4 elements it had starting at index 2.

	- Returns nothing, cannot fail.
	
- `vec_concat(void *vec_ptr, void *array, size_t length)`

- `vec_splice(void *vec_ptr, size_t offset, void *array, size_t length)`

	- _concatenate_, or _splice_ a vector with an array.

	- Array should be an array of the same type as the vector.

	- Length is the length of the array in type-units, not bytes
	  ('concat'ing (or 'splice'ing) an array of five ints, length would be `5`, not `20` or however big fives ints are).

	- returns `0` on succes, `ENOMEM` when out of memory, `EOVERFLOW` if something overflows.

- `vec_copy(void *dest_ptr, void *src)`
- `vec_transfer(void *dest_ptr, void *src, size_t nmemb)`
	- copy elements to a vector from a source vector (`vec_copy()`) or a source array (`vec_transfer()`)

	- returns `0` on succes, `ENOMEM` when out of memory, `EOVERFLOW` if something overflows.

	- **note** the functionality of `vec_transfer()` is likely to be completely changed in the near future

- `vec_join(void *dest_ptr, void *src)`

	- _join_ two vectors

	- behaves equivalently to `vec_concat()`, assuming `src` is a valid vector

	- returns `0` on success, `ENOMEM` when out of memory, `EOVERFLOW` if something overflows

- `vec_clone(void *vec)`

	- returns a copy of the vector, or the null pointer if allocation fails

### fun

- `vec_foreach(variable, vector)` is a macro which loops over the elements of `vector`,
assigning the address of each one to `varible` in sequence. e.g.

```c
int
main()
{
	int *foo;

	vec_ctor(foo);
	if (!foo) abort();

	/* ... */

	vec_foreach(int *each, foo) {
		printf("%d * 2 == %d\n", *each, *each + *each);
	}

	/* ... */

	return 0;
}
```
	- as seen above, you can declare variables inside vec_foreach(), like a for loop initializer

	- furthermore, `continue` and `break` behave as expected

	- finally, `vec_foreach()` contains no double evaluatation.
	any correctly typed expression can be used as `vector` or `variable`, even if it has side-effects

# pitfalls

- forgeting the `&`

	- as stated above, the mutating functions (insert, delete, et al.) take a *pointer* to a vector,
rather than the vector itself.

	- it's very easy to forget the `&` and write something like `vec_append(vec, foo)`
which gives strange results for a vector of pointers (and a build error otherwise)

- passing a vector to a fucntion

	- this seems like reasonable code

```c

int
frobnicate(struct frob *target, long errata)
{
	int err;

	/* ... */

	err = vec_append(&target, some_local)
	if (err) return err;

	/* ... */

	return 0;
}
```

	- however, it has a fatal bug -- if target overflows it's capacity, it will be reallocated and
quite likely moved to another address. these changes will not propagate back to the caller,
resulting in a user-after-free bug

the fix is simply pass a pointer the vector, as the vec.h functions do

- using compound literals

	- it's convenient to use compound literals, particular when the vector is of scalars --
the scope need not be polluted with temporary variables. however, there is wrinkle when using this
idiom with structs or arrays

```c
	vec_insert(frob, index, (struct frob[]) {{ .a = res_a, .b = res_b, .c = res_c }});
```

	- this snippet looks reasonable, but will not compile. compilers vary in helpfulness, but the source error
comes from the preprocessor -- the commas are interpreted as argument delimiters, rather than a pat of a single initializer

the fix is simply to wrap it in paranthese

```c
	vec_insert(frob, index, ((struct frob[]) {{ .a = res_a, .b = res_b, .c = res_c }}));
```

## set

declare a crit-bit tree with `struct set *instance`

alloc & initialize:

- `instance = set_alloc()`

free:

- `set_free(instance)`

### supported operations:

note that all of these functions have two variations ----
one takes a byte buffer and a length,
and one takes a null-terminated c-string.
Both of them have equivalent behavior, what differs is how length is calculated.

**adding members:**

- `set_add_bytes(struct set *set, void *elem, size_t length)`

- `set_add_string(struct set *set, char *elem)`

	- *insert* `elem` into `set`

	- return `0` if succesful,
`ENOMEM` if out of memory,
`EEXIST` if `elem` was already in `set`,
`EILSEQ` if `elem` is a prefix of an item already in `set`,
`EOVERFLOW` if `elem` is too large to be added,
`EFAULT` if given the null pointer as an argument,
`EINVAL` if the length of `elem` is zero,


- `set_remove_bytes(struct set *set, void *elem, size_t length)`

- `set_remove_string(struct set *set, char *elem)`

	- *remove* `elem` from `set`

	- return `0` if successful
`ENOENT` if the `elem` is not in `set`,
`EFAULT` if given the null pointer as an argument,

- `set_contains_bytes(struct set *set, void *elem, size_t length)`

- `set_contains_string(struct set *set, char *elem, size_t length)`

	- check if `elem` is contained in `set`

	- return `true` iff `elem` is in `set`, `false` otherwise

- `set_prefix_bytes(struct set *set, void *prefix, size_t length)`

- `set_prefix_string(struct set *set, char *prefix)`

	- check if any elements of `set` have the prefix `prefix`

	- return `true` iff so, `false` otherwise

- `set_query_bytes(void ***out, size_t nmemb, struct set *set, void *prefix, size_t length)`

- `set_query_string(void ***out, size_t nmemb, struct set *set, char *prefix)`

- `set_query_vector(void ***out, size_t nmemb, struct set *set, void *prefix)`

	- return the number of elements in `set` with the prefix `prefix`

	- if `out` is not the null pointer,
the array of `nmemb` elements it points to will be filled with the elements containing the prefix,
potentially truncated if `nmemb` is less than the total number of elements with that prefix

	- as a special case, if `out` points to the null pointer,
a sufficiently large array will be allocated
and the pointer pointed to by `out` will be mutated to be the allocated array
