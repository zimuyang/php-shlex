--TEST--
Test syntax split redirect

--FILE--
<?php 

include_once __DIR__ . '/../shlex_test.php';

$obj = new ShlexTest();
$obj->setUp();
$obj->testSyntaxSplitRedirect();

echo "Done\n";
?>
--EXPECT--
Done
