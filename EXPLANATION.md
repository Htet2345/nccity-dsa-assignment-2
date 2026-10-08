# Assignment 2: XML structure validator

XML (eXtensible Markup Language) stores data in named elements. An element normally
has an opening tag, content, and a matching closing tag, for example
`<to>Tove</to>`. Elements may contain other elements, but must close in reverse
order: `<a><b></b></a>` is properly nested, while `<a><b></a></b>` is not. Tag
names are case-sensitive, and a document has one root element. Empty elements
can use a self-closing tag such as `<item />`.

The program reads `note.xml` by default, matching the supplied demo. A different
file can be passed as a command-line argument. It scans the file from left to
right and uses an array-based stack to remember open elements. The `Tag`
structure stores a tag's name and opening line; the `Stack` structure contains an
array of those tags and a count. `Parser` holds the current position and error.

When an opening tag is found, `push()` adds it to the stack. For a closing tag,
`close_tag()` compares its name with the top entry. A match removes that entry;
a mismatch or an empty stack means the tags are invalid. Self-closing tags do
not need a stack entry. At the end, the stack must be empty and exactly one root
element must have been found. Functions also read tag names and quoted attributes
and skip comments, processing instructions, and CDATA so their contents are not
mistaken for element tags.

The output begins with `XML is valid` or `XML is invalid`, as in the demo.
Invalid input also produces a line/column diagnostic. The file buffer grows with
`realloc()`, and both the buffer and stack are freed before the program exits.
Scanning takes O(n) time for a file of n bytes; reading the whole file uses O(n)
memory plus a bounded stack. Names are limited to 255 ASCII characters and
nesting to 1024 open tags; exceeding either limit produces an error.

This is an educational **tag-structure validator**, not a complete XML parser.
It does not validate entities, duplicate attributes, namespaces, XML declaration
grammar/placement, Unicode names/encoding, DTDs, or XSD schemas. DOCTYPE/DTD
declarations are explicitly rejected as unsupported. A successful result means
the supported tag structure passes, rather than full XML 1.0 conformance.

Research reference: [W3C XML 1.0 specification, sections 2.1 and 3.1](https://www.w3.org/TR/xml/).
