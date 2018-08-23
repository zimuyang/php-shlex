--TEST--
Test syntax split paren

--FILE--
<?php 

include_once __DIR__ . '/../shlex_test.php';

$obj = new ShlexTest();
$obj->setUp();
$obj->testSyntaxSplitParen();

echo "Done\n";
?>
--EXPECT--
Done
