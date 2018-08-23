--TEST--
Test punctuation with posix

--FILE--
<?php 

include_once __DIR__ . '/../shlex_test.php';

$obj = new ShlexTest();
$obj->setUp();
$obj->testPunctuationWithPosix();

echo "Done\n";
?>
--EXPECT--
Done
