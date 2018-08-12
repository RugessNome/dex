

# `Dex`

## Commands syntax

There are two main syntax for commands:
> \\*name* *arguments*

> \\*name*[*brackets-arguments*] *arguments*

Some conventions about arguments:
- `<arg-name>` denotes a single-word argument;
- `(arg-name)` denotes an argument that span over a line;
- `{arg-name}` denotes an argument that extends until the next paragraph.

**Examples:**:
- `\rect <x> <y> <w> <h>` denotes a command taking four parameters, each one 
  being a single word;
- `\brief (text)` denotes a command taking a single parameter that 
  spans over a complete line;
- `\indent {text}` denotes a command taking a single parameter that extends until 
  the next paragraph.

**Note:** if a command takes more than one parameter, then all arguments are 
single word, i.e. only a single parameter can span over more than a word.

**Note:** you may extend the span of an argument by surrounding it with braces `{}`.

**Example:**
- `\brief {This text is allowed to span over multiple lines}`

Brackets parameters are parameters provided inside brackets (huh?). 
They can have a name and may be optional.

Syntax:
> name = value

> value

Brackets arguments can only be:
- booleans, i.e. `true` or `false`;
- integers, e.g. -42, +33, etc..
- or strings, e.g. "Hello World !"

**Note:** For strings, quotes may be omitted if your string does not contain any 
comma or equal sign, in which case trailing spaces are ignored.

Each bracket argument can have a name, that name cannot contain any comma 
or equal sign, but may contains spaces.

**Examples:**
- `\cmd[a=5,b=2] arguments...`
- `\cmd[5, b = Hello World] arguments...`
- `\cmd[show legend=true, title="A simple graph, yet beautiful"] arguments..`


## Environments

Two special commands named `\begin` and `\end` can be used to enter and leave 
an environment.

Within an environment, new commands may be available, while some other may become 
inaccessible. 