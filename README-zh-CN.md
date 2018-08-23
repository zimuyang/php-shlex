## [English document](README.md)  |  [中文 文档](README-zh-CN.md)

# Shlex 

Shlex 是用 C 语言编写的 PHP 扩展。该扩展实现了 python 中 shlex 库的功能。为方便用户更好的熟悉 Shlex 扩展，该扩展实现的类在属性和方法名称，以及使用方式上与 python 的 shlex 库基本保持一致。接口文档也采用 python 的 shlex 库接口文档修改而来。

Shlex 令为类似于 Unix shell 的简单语法编写词法分析器更方便。这通常用于编写微语言或用于解析引用的字符串。

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
目前不支持windows系统。



# <span id="functions">Functions</span>

### <span id="shlex_split">shlex_split</span>

使用类 shell 语法拆分字符串。

##### Description

```
array shlex_split( string|resource|null $s [, bool $comments = false [, bool $posix = true ]] )
```

##### Parameters

###### s

&nbsp;&nbsp;&nbsp;&nbsp;使用类 shell 语法拆分字符串 s。

```
注意：
由于 shlex_split() 函数实例化一个 Shlex 实例，传递 null 给 s 将读取要从标准输入拆分的字符串。
```

###### comments

&nbsp;&nbsp;&nbsp;&nbsp;如果 comments 是 false（默认值），则将禁用对给定字符串中的注释的解析（将 Shlex 实例的 commenters 属性设置为空字符串）。

###### posix

&nbsp;&nbsp;&nbsp;&nbsp;此函数默认在 POSIX 模式下运行，但如果 posix 参数为 false 则使用非 POSIX 模式。

##### Return Values

返回拆分后的字符串数组。

##### Examples

```
<?php

$s = "foo#bar";
$ret = shlex_split($s, true);

var_dump($ret);

?>
```

上面的例子将输出：

```
array(1) {
  [0] =>
  string(3) "foo"
}
```

<br>

### <span id="shlex_quote">shlex_quote</span>

返回字符串 s 的 shell 转义版本。

##### Description

```
string shlex_quote( string $s )
```




##### Parameters

###### s

&nbsp;&nbsp;&nbsp;&nbsp;待转义的字符串。


##### Return Values

返回的值是一个字符串，可以安全地用作 shell 命令行中的一个令牌，用于不能使用列表的情况。

##### Examples

```
<?php

// 该输出内容如果被执行，将会导致index.php文件被删除
$filename = "somefile; rm -rf index.php";
$command = sprintf("ls -l %s", $filename);
echo $command;
echo "\n";

// shlex_quote() 堵住了该漏洞
$command = sprintf("ls -l %s", shlex_quote($filename));
echo $command;
echo "\n";

// 远程连接
$remoteCommand = sprintf("ssh home %s", shlex_quote($command));
echo $remoteCommand;
echo "\n";

?>
```

上面的例子将输出：

```
ls -l somefile; rm -rf index.php
ls -l 'somefile; rm -rf index.php'
ssh home 'ls -l '"'"'somefile; rm -rf index.php'"'"''
```




# <span id="classes-and-methods">Classes and methods</span>

### <span id="shlex">Shlex</span>

##### Introduction

Shlex 实例或子类实例是词法分析器对象。

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

&nbsp;&nbsp;&nbsp;&nbsp;此 Shlex 实例正在读取字符的输入流。

###### <span id="shlex-infile">infile</span>

&nbsp;&nbsp;&nbsp;&nbsp;当前输入文件的名称，最初在类实例化时设置或由以后的源请求堆叠。在构造错误消息时检查这可能是有用的。

###### <span id="shlex-eof">eof</span>

&nbsp;&nbsp;&nbsp;&nbsp;令牌用于确定文件结束。这将在非 POSIX 模式下设置为空字符串（''），在 POSIX 模式下设置为 null。

###### <span id="shlex-commenters">commenters</span>

&nbsp;&nbsp;&nbsp;&nbsp;被识别为注释标签的字符串。将忽略该标签到行尾的所有字符。默认情况下只包括 '#'。

###### <span id="shlex-wordchars">wordchars</span>

&nbsp;&nbsp;&nbsp;&nbsp;将累积为多字符令牌的字符串。默认情况下，包括所有 ASCII 字母数字和下划线。在POSIX模式下，拉丁文-1集中的重音字符也包括在内。如果 punctuationChars 不为空，那么可能出现在文件名规范和命令行参数中的字符 ~-./*?= 也将包括在此属性中，并且出现在 punctuationChars 中的任何字符将从 wordchars 中删除（如果它们存在）。

###### <span id="shlex-whitespace">whitespace</span>

&nbsp;&nbsp;&nbsp;&nbsp;将被视为空格并被跳过的字符。空白限制令牌。默认情况下，包括空格，制表符，换行符和回车。

###### <span id="shlex-whitespace-split">whitespaceSplit</span>

&nbsp;&nbsp;&nbsp;&nbsp;如果 true，令牌将只拆分在空格。这是有用的，例如，用 Shlex 解析命令行，以类似于 shell 参数的方式获取令牌。如果此属性是 true，则 punctuationChars 将不起作用，并且拆分将仅在空格上发生。当使用 punctuationChars （旨在提供更接近 shell 实现的解析）时，建议将 whitespaceSplit 保留为 false （默认值）。


###### <span id="shlex-quotes">quotes</span>

&nbsp;&nbsp;&nbsp;&nbsp;将被视为字符串引号的字符。令牌累积，直到再次遇到相同的引号（因此，不同的引号类型在 shell 中彼此保护。）默认情况下，包括 ASCII 单引号和双引号。

###### <span id="shlex-escape">escape</span>

&nbsp;&nbsp;&nbsp;&nbsp;将被视为逃逸的字符。这将仅用于 POSIX 模式，并且只包括 '\' 默认情况下。

###### <span id="shlex-escapedquotes">escapedquotes</span>

&nbsp;&nbsp;&nbsp;&nbsp;quotes 中将解释 escape 中定义的转义字符的字符。这只在 POSIX 模式下使用，默认情况下只包括 '"'。

###### <span id="shlex-lineno">lineno</span>

&nbsp;&nbsp;&nbsp;&nbsp;源行号（到目前为止看到的换行次数加一）。

###### <span id="shlex-debug">debug</span>

&nbsp;&nbsp;&nbsp;&nbsp;如果此属性是数字和 1 或更多，Shlex 实例将打印详细的进度输出其行为。如果你需要使用这个，你可以阅读模块的源代码来了解细节。

###### <span id="shlex-token">token</span>

&nbsp;&nbsp;&nbsp;&nbsp;令牌缓冲区。在捕获异常时检查这可能是有用的。

###### <span id="shlex-source">source</span>

&nbsp;&nbsp;&nbsp;&nbsp;默认情况下，此属性为 null。如果为其分配字符串，那么该字符串将被识别为词汇级包含请求，类似于各种shell中的 source 关键字。也就是说，紧随其后的令牌将作为文件名打开，并且将从该流中获取输入，直到EOF，在该点处将调用该流的 fclose() 方法，并且输入源将再次变为原始输入流。源请求可以被堆叠在任何数量级的深度。

###### <span id="shlex-punctuation-chars">punctuationChars</span>

&nbsp;&nbsp;&nbsp;&nbsp;将被视为标点符号的字符。标点符号的运行将作为单个标记返回。然而，请注意，不会执行语义有效性检查：例如，“>>>” 可以作为令牌返回，即使它可能不被 shell 识别。

### <span id="shlex-methods">Methods</span>

### <span id="shlex-__construct">Shlex::__construct</span>

构造函数

##### Description

```
public void function Shlex::__construct( [ string|resource|null $instream = null [, string|null $infile = null [, bool $posix = false [, string|bool|null $punctuationChars = false ]]]])
```

##### Parameters

###### instream

&nbsp;&nbsp;&nbsp;&nbsp;instream 参数（如果存在）指定从哪里读取字符。它必须是资源类型（能被 fread( ) 读取），或字符串。如果没有给出参数，将从 php://stdin 获取输入。

###### infile

&nbsp;&nbsp;&nbsp;&nbsp; infile 参数是文件名字符串，它设置 infile 属性的初始值。如果 instream 参数为 null, 则 infile 始终为 null。

###### posix

&nbsp;&nbsp;&nbsp;&nbsp;posix 参数定义操作模式：当 posix 为 false（默认）时，shlex 实例将以兼容模式运行。当在 POSIX 模式下操作时，shlex 将尝试尽可能接近 POSIX 外壳解析规则。

###### punctuationChars

&nbsp;&nbsp;&nbsp;&nbsp;punctuationChars 参数提供了一种使行为更接近实际 shell 解析的方式。这可以采取一些值：默认值，false。如果设置为 true，则更改字符 ();<>|& 的解析：将这些字符（被视为标点符号）的任何运行作为单个令牌返回。如果设置为非空字符串，那些字符将用作标点符号。在 punctuationChars 中出现的 wordchars 属性中的任何字符将从 wordchars 中删除。

##### Return Values

无返回值。

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

上面的例子将输出：

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

析构函数

##### Description

```
public void function Shlex::__destruct( void )
```

用于释放 Shlex 对象所持有的资源对象。内部会调用 fclose( ) 关闭文件句柄。 


##### Parameters

无参数。

##### Return Values

无返回值。

##### Examples

无示例。

<br>


### <span id="shlex-key">Shlex::key</span>

只为实现 Iterator 接口的 key 方法，无实际用途。

##### Description

```
public void function Shlex::key( void )
```

##### Parameters

无参数。

##### Return Values

无返回值。

##### Examples

无示例。

<br>


### <span id="shlex-next">Shlex::next</span>

只为实现 Iterator 接口的 next 方法，无实际用途。

##### Description

```
public void function Shlex::next( void )
```

##### Parameters

无参数。

##### Return Values

无返回值。

##### Examples

无示例。

<br>


### <span id="shlex-rewind">Shlex::rewind</span>

只为实现 Iterator 接口的 rewind 方法，无实际用途。

##### Description

```
public void function Shlex::rewind( void )
```

##### Parameters

无参数。

##### Return Values

无返回值。

##### Examples

无示例。

<br>


### <span id="shlex-current">Shlex::current</span>

返回 Shlex 本次迭代读取的 token 值。

##### Description

```
public string|null function Shlex::current( void )
```

##### Parameters

无参数。

##### Return Values

返回 Shlex 本次迭代读取的 token 值。

##### Examples

无示例。

<br>


### <span id="shlex-valid">Shlex::valid</span>

判断本次迭代是否有效。

##### Description

```
public bool function Shlex::valid( void )
```

##### Parameters

无参数。

##### Return Values

如果返回 true 则有效，false 则无效。

```
注意：
由于该类实现的原因，迭代读取下一个元素也在该方法内部被调用。所以 next() 方法是无效的。
```

##### Examples

无示例。

<br>


### <span id="shlex-push-token">Shlex::pushToken</span>

将参数推送到令牌堆栈。

##### Description

```
public void function Shlex::pushToken( string $tok )
```

##### Parameters

###### tok

&nbsp;&nbsp;&nbsp;&nbsp;被推送的参数。

##### Return Values

无返回值。

##### Examples

无示例。

<br>


### <span id="shlex-push-source">Shlex::pushSource</span>

将输入源流推送到输入堆栈。

##### Description

```
public void function Shlex::pushSource( string|resource $newstream, string|null $newfile = null );
```

##### Parameters

###### newstream

&nbsp;&nbsp;&nbsp;&nbsp;被推送的输入源流。

###### newfile

&nbsp;&nbsp;&nbsp;&nbsp;如果指定了 filename 参数，它以后将可用于错误消息。这是 sourcehook( ) 方法内部使用的相同方法。

##### Return Values

无返回值。

##### Examples

无示例。

<br>


### <span id="shlex-pop-source">Shlex::popSource</span>

从输入堆栈弹出最后推入的输入源。这是当词法分析器在堆叠输入流上到达 EOF 时内部使用的相同方法。

##### Description

```
public void function Shlex::popSource( void )
```

##### Parameters

无参数。

##### Return Values

无返回值。

##### Examples

无示例。

<br>


### <span id="shlex-get-token">Shlex::getToken</span>

返回令牌。

##### Description

```
public string|null|ShlexException function Shlex::getToken( void )
```

##### Parameters

无参数。

##### Return Values

返回令牌。如果令牌已使用 pushToken( ) 堆叠，请从堆栈弹出令牌。否则，从输入流中读取一个。如果读取遇到立即文件结束，则返回 eof （在非POSIX模式下为空字符串（''），在POSIX模式下为 null）。

##### Examples

无示例。

<br>


### <span id="shlex-read-token">Shlex::readToken</span>

读取原始令牌。

##### Description

```
public string|null|ShlexException function Shlex::readToken( void )
```

读取原始令牌。忽略后推堆栈，并且不解释源请求。 （这通常不是一个有用的切入点，在这里只是为了完整性的记录。）

##### Parameters

无参数

##### Return Values

返回原始令牌。

##### Examples

无示例。

<br>


### <span id="shlex-sourcehook">Shlex::sourcehook</span>

##### Description

```
public array function Shlex::sourcehook( string $newfile )
```

当 Shlex 检测到源请求（参见下面的 source）时，该方法被赋予以下令牌作为参数，并且期望返回由文件名和类似打开文件的对象组成的数组。

通常，此方法首先剥离参数的任何引号。如果结果是绝对路径名，或者没有生效的上一个源请求，或者上一个源是流（例如 php://stdin），则结果将保留。否则，如果结果是相对路径名，则在源包含堆栈上紧接着的文件的名称的目录部分被添加在前面（该行为类似于 C 预处理器处理 #include "file.h" 的方式）。

操作的结果被视为文件名，并作为数组的第一个组件返回，fopen( ) 调用它来生成第二个组件。 （注意：这是与实例初始化中的参数顺序相反的！）

这个钩子是暴露的，所以你可以使用它来实现目录搜索路径，添加文件扩展名等。没有相应的“关闭”钩子，但是当它返回 EOF 时，Shlex 实例将调用源输入流的 fclose( ) 方法。

为了更明确地控制源堆叠，请使用 pushSource( ) 和 popSource( ) 方法。

##### Parameters

###### newfile

&nbsp;&nbsp;&nbsp;&nbsp;文件路径。

##### Return Values

返回由文件名和类似打开文件的对象组成的数组。

##### Examples

无示例。

<br>


### <span id="shlex-error-leader">Shlex::errorLeader</span>

返回类似C编译器，Emacs友好的错误引导信息。

##### Description

```
public string function Shlex::errorLeader( string $infile = null, int|null $lineno = null )
```

此方法以 Unix C 编译器错误标签的格式生成错误消息引导程序；格式为 '"%s", line %d: '，其中 %s 用当前源文件的名称替换，%d 用当前输入行号替换（可选参数可用于覆盖这些）。

提供这种便利是为了鼓励 Shlex 用户以 Emacs 和其他 Unix 工具理解的标准，可分析格式生成错误消息。


##### Parameters

###### infile

&nbsp;&nbsp;&nbsp;&nbsp;当前源文件的名称。

###### lineno

&nbsp;&nbsp;&nbsp;&nbsp;前输入行号。

##### Return Values

返回类似 C 编译器，Emacs 友好的错误引导信息。

##### Examples

无示例。

<br>


### <span id="shlex-exception">ShlexException</span>

Shlex的异常类

##### Introduction

该类主要用于 Shlex 类内部执行错误时，抛出的异常。

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
