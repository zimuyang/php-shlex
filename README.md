## [English document](README.md)  |  [中文 文档](README-zh-CN.md)

# Shlex 

Shlex is a PHP extension written in C. This extension implements the functionality of the shlex library in Python. In order to make users more familiar with the Shlex extension, the class implemented by the extension is basically the same as the python shlex library in terms of property and method names. The interface documentation is also modified from the python shlex library interface documentation.

The Shlex makes it easy to write lexical analyzers for simple syntaxes resembling that of the Unix shell. This will often be useful for writing minilanguages or for parsing quoted strings.

# Table of contents

1. [Requirement](#requirement)
1. [Installation](#installation)
   * [Installation on Linux/OSX](#installation-on-linux-or-osx)
   * [For windows](#for-windows)
1. [Functions](#functions)
   * [shlex_split](#shlex_split)
   * [shlex_quote](#shlex_quote)
1. [Classes and methods](#classes-and-methods)
   * [Shlex](#shlex)
      * [Properties](#shlex-properties)
       * [instream](#shlex-instream)
       * [infile](#shlex-infile)
       * [posix](#shlex-posix)
       * [eof](#shlex-eof)
       * [commenters](#shlex-commenters)
       * [wordchars](#shlex-wordchars)
       * [whitespace](#shlex-whitespace)
       * [whitespaceSplit](#shlex-whitespace-split)
       * [quotes](#shlex-quotes)
       * [escape](#shlex-escape)
       * [escapedquotes](#shlex-escapedquotes)
       * [state](#shlex-state)
       * [pushback](#shlex-pushback)
       * [lineno](#shlex-lineno)
       * [debug](#shlex-debug)
       * [token](#shlex-token)
       * [filestack](#shlex-filestack)
       * [source](#shlex-source)
       * [punctuationChars](#shlex-punctuation-chars)
       * [_punctuationChars](#shlex-_punctuation-chars)
      * [Methods](#shlex-methods)
       * [Shlex::__construct](#shlex-__construct)
       * [Shlex::__destruct](#shlex-__destruct)
       * [Shlex::key](#shlex-key)
       * [Shlex::next](#shlex-next)
       * [Shlex::rewind](#shlex-rewind)
       * [Shlex::current](#shlex-current)
       * [Shlex::valid](#shlex-valid)
       * [Shlex::pushToken](#shlex-push-token)
       * [Shlex::pushSource](#shlex-push-source)
       * [Shlex::popSource](#shlex-pop-source)
       * [Shlex::getToken](#shlex-get-token)
       * [Shlex::readToken](#shlex-read-token)
       * [Shlex::sourcehook](#shlex-sourcehook)
       * [Shlex::errorLeader](#shlex-error-leader)
   * [ShlexException](#shlex-exception)



## <span id="requirement">Requirement</span>
- PHP 7.0 +
- Linux/OSX

## <span id="installation">Installation</span>

### <span id="installation-on-linux-or-osx">Installation on Linux/OSX</span>
```
phpize
./configure
make && make install
```
### <span id="for-windows">For windows</span>
Windows system is currently not supported.


# <span id="functions">Functions</span>


### <span id="shlex_split">shlex_split</span>

Split the string s using shell-like syntax.

##### Description

```
array shlex_split( string|resource|null $s [, bool $comments = false [, bool $posix = true ]] )
```

##### Parameters

###### s

&nbsp;&nbsp;&nbsp;&nbsp;Split the string s using shell-like syntax.

```
Note:
Since the shlex_split() function instantiates a shlex instance, passing null for s will read the string to split from standard input.
```

###### comments

&nbsp;&nbsp;&nbsp;&nbsp;If comments is false (the default), the parsing of comments in the given string will be disabled (setting the commenters attribute of the shlex instance to the empty string). 

###### posix

&nbsp;&nbsp;&nbsp;&nbsp;This function operates in POSIX mode by default, but uses non-POSIX mode if the posix argument is false.
##### Return Values

Returns an array of split strings.

##### Examples

```
<?php

$s = "foo#bar";
$ret = shlex_split($s, true);

var_dump($ret);

?>
```

The above example will output:

```
array(1) {
  [0] =>
  string(3) "foo"
}
```

<br>

### <span id="shlex_quote">shlex_quote</span>

Return a shell-escaped version of the string s.

##### Description

```
string shlex_quote( string $s )
```




##### Parameters

###### s

&nbsp;&nbsp;&nbsp;&nbsp;The string to be escaped.


##### Return Values

The returned value is a string that can safely be used as one token in a shell command line, for cases where you cannot use a list.

##### Examples

```
<?php

// If the output is executed, it will cause the index.php file to be deleted.
$filename = "somefile; rm -rf index.php";
$command = sprintf("ls -l %s", $filename);
echo $command;
echo "\n";

// shlex_quote() blocked the vulnerability
$command = sprintf("ls -l %s", shlex_quote($filename));
echo $command;
echo "\n";

// remote connection
$remoteCommand = sprintf("ssh home %s", shlex_quote($command));
echo $remoteCommand;
echo "\n";

?>
```

The above example will output:

```
ls -l somefile; rm -rf index.php
ls -l 'somefile; rm -rf index.php'
ssh home 'ls -l '"'"'somefile; rm -rf index.php'"'"''
```




# <span id="classes-and-methods">Classes and methods</span>

### <span id="shlex">Shlex</span>

##### Introduction

A Shlex instance or subclass instance is a lexical analyzer object.

##### Class synopsis

```
Shlex implements Iterator {
  
  /* Properties */
  public resource|null $instream = null;
    public string|null $infile = null;
    private bool|null $posix = null;
    public string|null $eof = null;
    public string $commenters = '#';
    public string $wordchars = 'abcdfeghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_';
    public string $whitespace = " \t\r\n";
    public bool $whitespaceSplit = false;
    public string $quotes = '\'"';
    public string $escape = '\\';
    public string $escapedquotes = '"';
    private string $state = ' ';
    private array $pushback = [];
    public int $lineno = 1;
    public int $debug = 0;
    public string $token = '';
    private array $filestack = [];
    public string|null $source = null;
    public string|null $punctuationChars = null;
    private array|null $_punctuationChars = null;
  
  /* Methods */
  public void function __construct( [ string|resource|null $instream = null [, string|null $infile = null [, bool $posix = false [, string|bool|null $punctuationChars = false ]]]]);

    public void function __destruct( void );

    public void function key( void );
    
    public void function next( void );
    
    public void function rewind( void );

    public string|null function current( void );

    public bool function valid( void );

    public void function pushToken( string $tok );

    public void function pushSource( string|resource $newstream, string|null $newfile = null );

    public void function popSource( void );

    public string|null|ShlexException function getToken( void );
    
    public string|null|ShlexException function readToken( void );
    
    public array function sourcehook( string $newfile );
    
    public string function errorLeader( string $infile = null, int|null $lineno = null );
}
```


#### <span id="shlex-properties">Properties</span>

###### <span id="shlex-instream">instream</span>

&nbsp;&nbsp;&nbsp;&nbsp;The input stream from which this Shlex instance is reading characters.

###### <span id="shlex-infile">infile</span>

&nbsp;&nbsp;&nbsp;&nbsp;The name of the current input file, as initially set at class instantiation time or stacked by later source requests. It may be useful to examine this when constructing error messages.

###### <span id="shlex-eof">eof</span>

&nbsp;&nbsp;&nbsp;&nbsp;Token used to determine end of file. This will be set to the empty string (''), in non-POSIX mode, and to null in POSIX mode.

###### <span id="shlex-commenters">commenters</span>

&nbsp;&nbsp;&nbsp;&nbsp;The string of characters that are recognized as comment beginners. All characters from the comment beginner to end of line are ignored. Includes just '#' by default.

###### <span id="shlex-wordchars">wordchars</span>

&nbsp;&nbsp;&nbsp;&nbsp;The string of characters that will accumulate into multi-character tokens. By default, includes all ASCII alphanumerics and underscore. In POSIX mode, the accented characters in the Latin-1 set are also included. If punctuationChars is not empty, the characters ~-./*?=, which can appear in filename specifications and command line parameters, will also be included in this attribute, and any characters which appear in punctuationChars will be removed from wordchars if they are present there.

###### <span id="shlex-whitespace">whitespace</span>

&nbsp;&nbsp;&nbsp;&nbsp;Characters that will be considered whitespace and skipped. Whitespace bounds tokens. By default, includes space, tab, linefeed and carriage-return.

###### <span id="shlex-whitespace-split">whitespaceSplit</span>

&nbsp;&nbsp;&nbsp;&nbsp;If true, tokens will only be split in whitespaces. This is useful, for example, for parsing command lines with Shlex, getting tokens in a similar way to shell arguments. If this attribute is true, punctuationChars will have no effect, and splitting will happen only on whitespaces. When using punctuationChars, which is intended to provide parsing closer to that implemented by shells, it is advisable to leave whitespaceSplit as false (the default value).


###### <span id="shlex-quotes">quotes</span>

&nbsp;&nbsp;&nbsp;&nbsp;Characters that will be considered string quotes. The token accumulates until the same quote is encountered again (thus, different quote types protect each other as in the shell.) By default, includes ASCII single and double quotes.

###### <span id="shlex-escape">escape</span>

&nbsp;&nbsp;&nbsp;&nbsp;Characters that will be considered as escape. This will be only used in POSIX mode, and includes just '\' by default.

###### <span id="shlex-escapedquotes">escapedquotes</span>

&nbsp;&nbsp;&nbsp;&nbsp;Characters in quotes that will interpret escape characters defined in escape. This is only used in POSIX mode, and includes just '"' by default.

###### <span id="shlex-lineno">lineno</span>

&nbsp;&nbsp;&nbsp;&nbsp;Source line number (count of newlines seen so far plus one).

###### <span id="shlex-debug">debug</span>

&nbsp;&nbsp;&nbsp;&nbsp;If this attribute is numeric and 1 or more, a Shlex instance will print verbose progress output on its behavior. If you need to use this, you can read the module source code to learn the details.

###### <span id="shlex-token">token</span>

&nbsp;&nbsp;&nbsp;&nbsp;The token buffer. It may be useful to examine this when catching exceptions.

###### <span id="shlex-source">source</span>

&nbsp;&nbsp;&nbsp;&nbsp;This attribute is null by default. If you assign a string to it, that string will be recognized as a lexical-level inclusion request similar to the source keyword in various shells. That is, the immediately following token will be opened as a filename and input will be taken from that stream until EOF, at which point the fclose() method of that stream will be called and the input source will again become the original input stream. Source requests may be stacked any number of levels deep.

###### <span id="shlex-punctuation-chars">punctuationChars</span>

&nbsp;&nbsp;&nbsp;&nbsp;Characters that will be considered punctuation. Runs of punctuation characters will be returned as a single token. However, note that no semantic validity checking will be performed: for example, ‘>>>’ could be returned as a token, even though it may not be recognised as such by shells.

### <span id="shlex-methods">Methods</span>



### <span id="shlex-__construct">Shlex::__construct</span>

Constructor

##### Description

```
public void function Shlex::__construct( [ string|resource|null $instream = null [, string|null $infile = null [, bool $posix = false [, string|bool|null $punctuationChars = false ]]]])
```

##### Parameters

###### instream

&nbsp;&nbsp;&nbsp;&nbsp;The instream argument, if present, specifies where to read characters from. It must be a resource type variable (can be read by fread( )), or a string. If no argument is given, input will be taken from php://stdin. 

###### infile

&nbsp;&nbsp;&nbsp;&nbsp; The second optional argument is a filename string, which sets the initial value of the infile attribute. If the instream argument is null, then this infile argument is always null.

###### posix

&nbsp;&nbsp;&nbsp;&nbsp;The posix argument defines the operational mode: when posix is false (default), the Shlex instance will operate in compatibility mode. When operating in POSIX mode, Shlex will try to be as close as possible to the POSIX shell parsing rules. 

###### punctuationChars

&nbsp;&nbsp;&nbsp;&nbsp;The punctuationChars argument provides a way to make the behaviour even closer to how real shells parse. This can take a number of values: the default value, false. If set to true, then parsing of the characters ();<>|& is changed: any run of these characters (considered punctuation characters) is returned as a single token. If set to a non-empty string of characters, those characters will be used as the punctuation characters. Any characters in the wordchars attribute that appear in punctuationChars will be removed from wordchars. 

##### Return Values

No value is returned.

##### Examples

```
<?php

$instance = new Shlex("a && b || c", null, false, "|");

$list = [];

foreach ($instance as $value) {
  $list[] = $value;
}

var_dump($list);

?>
```

The above example will output:

```
array(6) {
  [0] =>
  string(1) "a"
  [1] =>
  string(1) "&"
  [2] =>
  string(1) "&"
  [3] =>
  string(1) "b"
  [4] =>
  string(2) "||"
  [5] =>
  string(1) "c"
}
```

<br>


### <span id="shlex-__destruct">Shlex::__destruct</span>


Destructor

##### Description

```
public void function Shlex::__destruct( void )
```

Used to release resource objects held by Shlex objects. Internally, fclose( ) is called to close the file handle.


##### Parameters

No parameters.

##### Return Values

No value is returned.

##### Examples

No examples.

<br>


### <span id="shlex-key">Shlex::key</span>

There is no practical use for the key method of the Iterator interface.

##### Description

```
public void function Shlex::key( void )
```

##### Parameters

No parameters.

##### Return Values

No value is returned.

##### Examples

No examples.

<br>


### <span id="shlex-next">Shlex::next</span>

There is no practical use for the next method of the Iterator interface.

##### Description

```
public void function Shlex::next( void )
```

##### Parameters

No parameters.

##### Return Values

No value is returned.

##### Examples

No examples.

<br>


### <span id="shlex-rewind">Shlex::rewind</span>

There is no practical use for the rewind method of the Iterator interface.

##### Description

```
public void function Shlex::rewind( void )
```

##### Parameters

No parameters.

##### Return Values

No value is returned.

##### Examples

No examples.

<br>


### <span id="shlex-current">Shlex::current</span>

Returns the token value read by Shlex this iteration.

##### Description

```
public string|null function Shlex::current( void )
```

##### Parameters

No parameters.

##### Return Values

Returns the token value read by Shlex this iteration.

##### Examples

No examples.

<br>


### <span id="shlex-valid">Shlex::valid</span>

Determine if this iteration is valid.

##### Description

```
public bool function Shlex::valid( void )
```

##### Parameters

No parameters.

##### Return Values

Valid if true is returned, false is invalid.

```
Note:
Due to the implementation of this class, iteratively reading the next element is also called inside the method. So the next() method is invalid.
```

##### Examples

No examples.

<br>


### <span id="shlex-push-token">Shlex::pushToken</span>

Push the argument onto the token stack.

##### Description

```
public void function Shlex::pushToken( string $tok )
```

##### Parameters

###### tok

&nbsp;&nbsp;&nbsp;&nbsp;The parameter being pushed.

##### Return Values

No value is returned.

##### Examples

No examples.

<br>


### <span id="shlex-push-source">Shlex::pushSource</span>

Push an input source stream onto the input stack.

##### Description

```
public void function Shlex::pushSource( string|resource $newstream, string|null $newfile = null );
```

##### Parameters

###### newstream

&nbsp;&nbsp;&nbsp;&nbsp;The input source stream being pushed.

###### newfile

&nbsp;&nbsp;&nbsp;&nbsp;If the filename argument is specified it will later be available for use in error messages. This is the same method used internally by the sourcehook() method.



##### Return Values

No value is returned.

##### Examples

No examples.

<br>


### <span id="shlex-pop-source">Shlex::popSource</span>

Pop the last-pushed input source from the input stack. This is the same method used internally when the lexer reaches EOF on a stacked input stream.

##### Description

```
public void function Shlex::popSource( void )
```

##### Parameters

No parameters.

##### Return Values

No value is returned.

##### Examples

No examples.

<br>


### <span id="shlex-get-token">Shlex::getToken</span>

Return a token. 

##### Description

```
public string|null|ShlexException function Shlex::getToken( void )
```

##### Parameters

No parameters.

##### Return Values

If tokens have been stacked using pushToken(), pop a token off the stack. Otherwise, read one from the input stream. If reading encounters an immediate end-of-file, eof is returned (the empty string ('') in non-POSIX mode, and null in POSIX mode).

##### Examples

No examples.

<br>


### <span id="shlex-read-token">Shlex::readToken</span>

Read a raw token.

##### Description

```
public string|null|ShlexException function Shlex::readToken( void )
```

Read a raw token. Ignore the pushback stack, and do not interpret source requests. (This is not ordinarily a useful entry point, and is documented here only for the sake of completeness.)


##### Parameters

No parameters.

##### Return Values

Return a raw token.

##### Examples

No examples.

<br>


### <span id="shlex-sourcehook">Shlex::sourcehook</span>


##### Description

```
public array function Shlex::sourcehook( string $newfile )
```

When Shlex detects a source request (see source below) this method is given the following token as argument, and expected to return a array of a filename and an open file-like object.

Normally, this method first strips any quotes off the argument. If the result is an absolute pathname, or there was no previous source request in effect, or the previous source was a stream (such as php://stdin), the result is left alone. Otherwise, if the result is a relative pathname, the directory part of the name of the file immediately before it on the source inclusion stack is prepended (this behavior is like the way the C preprocessor handles #include "file.h").

The result of the manipulations is treated as a filename, and returned as the first component of the tuple, with fopen() called on it to yield the second component. (Note: this is the reverse of the order of arguments in instance initialization!)

This hook is exposed so that you can use it to implement directory search paths, addition of file extensions, and other namespace hacks. There is no corresponding ‘close’ hook, but a shlex instance will call the fclose() method of the sourced input stream when it returns EOF.

For more explicit control of source stacking, use the pushSource() and popSource() methods.

##### Parameters

###### newfile

&nbsp;&nbsp;&nbsp;&nbsp;file path.

##### Return Values

Return a array of a filename and an open file-like object.

##### Examples

No examples.

<br>


### <span id="shlex-error-leader">Shlex::errorLeader</span>

Return an error message leader in the format of a Unix C compiler error label.

##### Description

```
public string function Shlex::errorLeader( string $infile = null, int|null $lineno = null )
```

This method generates an error message leader in the format of a Unix C compiler error label; the format is '"%s", line %d: ', where the %s is replaced with the name of the current source file and the %d with the current input line number (the optional arguments can be used to override these).

This convenience is provided to encourage Shlex users to generate error messages in the standard, parseable format understood by Emacs and other Unix tools.

##### Parameters

###### infile

&nbsp;&nbsp;&nbsp;&nbsp;The name of the current source file.

###### lineno

&nbsp;&nbsp;&nbsp;&nbsp;The current input line number.

##### Return Values

Return an error message leader in the format of a Unix C compiler error label.

##### Examples

No examples.

<br>


### <span id="shlex-exception">ShlexException</span>

Shlex's exception class

##### Introduction

This class is primarily used for exceptions thrown when the Shlex class internally performs an error.

##### Class synopsis

```
ShlexException extends Exception {}
```

##### Examples

```
<?php

  throw new ShlexException('No escaped character');

?>
```
