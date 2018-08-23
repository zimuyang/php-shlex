--TEST--
Test punctuation in word chars

--FILE--
<?php 

include_once __DIR__ . '/../shlex_test.php';

$obj = new ShlexTest();
$obj->setUp();
$obj->testPunctuationInWordChars();

echo "Done\n";
?>
--EXPECT--
Done
