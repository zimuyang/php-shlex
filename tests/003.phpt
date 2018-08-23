--TEST--
Test syntax split ampersand and pipe

--FILE--
<?php 

include_once __DIR__ . '/../shlex_test.php';

$obj = new ShlexTest();
$obj->setUp();
$obj->testSyntaxSplitAmpersandAndPipe();

echo "Done\n";
?>
--EXPECT--
Done
