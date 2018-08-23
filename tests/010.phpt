--TEST--
Test punctuation with whitespace split

--FILE--
<?php 

include_once __DIR__ . '/../shlex_test.php';

$obj = new ShlexTest();
$obj->setUp();
$obj->testPunctuationWithWhitespaceSplit();

echo "Done\n";
?>
--EXPECT--
Done
