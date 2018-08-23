--TEST--
Test syntax split custom

--FILE--
<?php 

include_once __DIR__ . '/../shlex_test.php';

$obj = new ShlexTest();
$obj->setUp();
$obj->testSyntaxSplitCustom();

echo "Done\n";
?>
--EXPECT--
Done
