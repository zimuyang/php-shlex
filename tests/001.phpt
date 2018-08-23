--TEST--
Test split posix

--FILE--
<?php 

include_once __DIR__ . '/../shlex_test.php';

$obj = new ShlexTest();
$obj->setUp();
$obj->testSplitPosix();

echo "Done\n";
?>
--EXPECT--
Done
