--TEST--
Test compat

--FILE--
<?php 

include_once __DIR__ . '/../shlex_test.php';

$obj = new ShlexTest();
$obj->setUp();
$obj->testCompat();

echo "Done\n";
?>
--EXPECT--
Done
