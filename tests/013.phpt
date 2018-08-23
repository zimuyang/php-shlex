--TEST--
Test quote

--FILE--
<?php 

include_once __DIR__ . '/../shlex_test.php';

$obj = new ShlexTest();
$obj->setUp();
$obj->testQuote();

echo "Done\n";
?>
--EXPECT--
Done
