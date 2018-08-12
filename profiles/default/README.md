

# `Dex` profile: `default`

> This document describes Dex default profile; that is, the available 
> commands and their meaning.

> This document assumes you are familiar with the concepts and syntax of 
> Dex as describe in the `doc` folder.

## Documenting programming entities

These following commands are used to document programming entities:

### `\fun (signature)`

**Description:** starts the documentation of a function.
For now this can only be used to document member functions 
when `\class` has been called.

### `\class <name>` 

**Description:** Starts the documentation of a class, the documentation of the class
ranges to the end of the file, unless `\endclass` is called.


## General purpose commands

### `\brief (text)`

**Description:** Sets the brief description of a `\class` or `\fun`.


### `\param (text)`

**Description:** Describes a function parameter.
This command can be called multiple times within a `\fun` block 
to document each parameter.

**Example:**
```cpp
/*!
 * \fun int add(int a, int b);
 * \param the first number to add
 * \param the second number
 */
```

### `\returns (text)`

**Description:** Describes the return value of a function.

**Example:**
```cpp
/*!
 * \fun int add(int a, int b);
 * \returns The sum of its inputs.
 */
```

## Text formatting commands

### `\b <word>`

**Description:** Puts the word in **bold**. 

**Note:** As usual, you can extend the span of this command by using braces.

**Examples:** 
- `This will put a single \b word in bold.`
- `A whole part of this sentence \b{will be in bold font}.`



## Environments

No environments are provided yet.

## Examples

**Examples:** 
```cpp
/*!
 * \fun double sqrt(double x);
 * \returns the square root
 * \param a real non-negative number
 * \brief Returns the square root of \a x.
 * 
 * This function uses Newton method to compute the square root 
 * iteratively.
 */
```


```cpp
# This block starts the documentation of a class.
/*!
 * \class Dir
 * \brief The Dir class represents a directory.
 */
# The class documentation ends at the end of the file 
# or when \endclass is called.
# Thus, the following blocks are member functions documentation.
/*!
 * \fun String absoluteFilePath() const;
 * \brief Returns the absolute file path of the directory.
 */
# The documentation of a function ends at the end of its block.
# This next block starts the documentation of another member function.
/*!
 * \fun static Dir current(); 
 * \brief Returns a Dir object representing the current working directory.
 */
```