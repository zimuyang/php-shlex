--TEST--
Test syntax split semicolon

--FILE--
<?php 

include_once __DIR__ . '/../shlex_test.php';

$obj = new ShlexTest();
$obj->setUp();
$obj->testSyntaxSplitSemicolon();

echo "Done\n";
?>
--EXPECT--
Done
