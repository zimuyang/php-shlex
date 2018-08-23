--TEST--
Test token types

--FILE--
<?php 

include_once __DIR__ . '/../shlex_test.php';

$obj = new ShlexTest();
$obj->setUp();
$obj->testTokenTypes();

echo "Done\n";
?>
--EXPECT--
Done
