--TEST--
Test empty string handling

--FILE--
<?php 

include_once __DIR__ . '/../shlex_test.php';

$obj = new ShlexTest();
$obj->setUp();
$obj->testEmptyStringHandling();

echo "Done\n";
?>
--EXPECT--
Done
