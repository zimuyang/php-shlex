<?php

mb_internal_encoding('UTF-8');
$data = file_get_contents(__DIR__ . '/tests/data');
$posixData = file_get_contents(__DIR__ . '/tests/posix_data');

class ShlexTest{

    const ASCII_LETTERS = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ';
    const DIGITS = '0123456789';

    private $data = null;
    private $posixData = null;

    public function assertEqual($first, $second, $msg = ''){
        if($first !== $second){
            throw new ShlexException($msg);
        }
    }

    public function assertNotIn($member, $container, $msg = ''){
        if(is_string($container)){
            if(strpos($container, $member) !== false){
                throw new ShlexException($msg);
            }
        }

        if(is_array($container)){
            if(in_array($member, $container)){
                throw new ShlexException($msg);
            }
        }
    }

    public function setUp(){
        global $data;
        global $posixData;

        $tmp = explode(PHP_EOL, $data);
        $this->data = [];

        foreach ($tmp as $x){
            $x = explode('|', $x);
            array_pop($x);
            $x[0] = str_replace('\n', "\n", $x[0]);
            $this->data[] = $x;
        }

        $tmp = explode(PHP_EOL, $posixData);
        $this->posixData = [];

        foreach ($tmp as $x){
            $x = explode('|', $x);
            array_pop($x);
            $x[0] = str_replace('\n', "\n", $x[0]);
            $this->posixData[] = $x;
        }
    }

    public function oldSplit($s){
        $ret = [];

        $handle = fopen('php://memory', 'r+');
        fwrite($handle, $s);
        rewind($handle);

        $lex = new Shlex($handle);
        $tok = $lex->getToken();

        while ($tok){
            $ret[] = $tok;
            $tok = $lex->getToken();
        }

        return $ret;
    }

    public function splitTest($data, $comments){
        foreach ($data as $v){
            $l = shlex_split($v[0], $comments);

            $s1 = var_export($v[0], true);
            $s2 = var_export($l, true);
            $s3 = var_export(array_slice($v, 1), true);

            $this->assertEqual($l, array_slice($v, 1), "{$s1}: {$s2} != $s3");
        }
    }

    public function testSplitPosix(){
        $this->splitTest($this->posixData, true);
    }

    public function testCompat(){
        foreach ($this->data as $v){
            $l = $this->oldSplit($v[0]);

            $s1 = var_export($v[0], true);
            $s2 = var_export($l, true);
            $s3 = var_export(array_slice($v, 1), true);

            $this->assertEqual($l, array_slice($v, 1), "{$s1}: {$s2} != $s3");
        }
    }

    public function testSyntaxSplitAmpersandAndPipe(){
        $tmp = ['&&', '&', '|&', ';&', ';;&', '||', '|', '&|', ';|', ';;|'];

        foreach ($tmp as $delimiter){

            $src = [
                "echo hi {$delimiter} echo bye",
                "echo hi{$delimiter}echo bye"
            ];

            $ref = ['echo', 'hi', $delimiter, 'echo', 'bye'];

            foreach ($src as $ss){

                $s = new Shlex($ss, null, false, true);
                $list = [];

                foreach ($s as $v){
                    $list[] = $v;
                }

                $this->assertEqual($ref, $list, "While splitting '{$ss}'");
            }
        }
    }

    public function testSyntaxSplitSemicolon(){
        $tmp = [';', ';;', ';&', ';;&'];

        foreach ($tmp as $delimiter){
            $src = [
                "echo hi {$delimiter} echo bye",
                "echo hi{$delimiter} echo bye",
                "echo hi{$delimiter}echo bye"
            ];

            $ref = ['echo', 'hi', $delimiter, 'echo', 'bye'];

            foreach ($src as $ss){
                $s = new Shlex($ss, null, false, true);

                $list = [];

                foreach ($s as $v){
                    $list[] = $v;
                }

                $this->assertEqual($ref, $list, "While splitting '{$ss}'");
            }
        }
    }

    public function testSyntaxSplitRedirect(){
        $tmp = ['<', '|'];

        foreach ($tmp as $delimiter){
            $src = [
                "echo hi {$delimiter} out",
                "echo hi{$delimiter} out",
                "echo hi{$delimiter}out"
            ];

            $ref = ['echo', 'hi', $delimiter, 'out'];

            foreach ($src as $ss){
                $s = new Shlex($ss, null, false, true);

                $list = [];

                foreach ($s as $v){
                    $list[] = $v;
                }

                $this->assertEqual($ref, $list, "While splitting '{$ss}'");
            }
        }
    }

    public function testSyntaxSplitParen(){
        $src = [
            '( echo hi )',
            '(echo hi)'
        ];

        $ref = ['(', 'echo', 'hi', ')'];

        foreach ($src as $ss){
            $s = new Shlex($ss, null, false, true);

            $list = [];

            foreach ($s as $v){
                $list[] = $v;
            }

            $this->assertEqual($ref, $list, "While splitting '{$ss}'");
        }
    }

    public function testSyntaxSplitCustom(){
        $ref = ['~/a', '&', '&', 'b-c', '--color=auto', '||', 'd', '*.py?'];
        $ss = "~/a && b-c --color=auto || d *.py?";
        $s = new Shlex($ss, null, false, "|");

        $list = [];

        foreach ($s as $v){
            $list[] = $v;
        }

        $this->assertEqual($ref, $list, "While splitting '{$ss}'");
    }

    public function testTokenTypes(){
        $source = 'a && b || c';
        $expected = [
            ['a', 'a'],
            ['&&', 'c'],
            ['b', 'a'],
            ['||', 'c'],
            ['c', 'a']
        ];

        $s = new Shlex($source, null, false, true);
        $observed = [];

        while (true){
            $t = $s->getToken();

            if($t === $s->eof){
                break;
            }

            if(strpos($s->punctuationChars, $t[0]) !== false){
                $tt = 'c';
            }else{
                $tt = 'a';
            }

            $observed[] = [$t, $tt];
        }

        $this->assertEqual($observed, $expected);
    }

    public function testPunctuationInWordChars(){
        $s = new Shlex('a_b__c', null, false, '_');

        $this->assertNotIn('_', $s->wordchars);

        $list = [];

        foreach ($s as $v){
            $list[] = $v;
        }

        $this->assertEqual($list, ['a', '_', 'b', '__', 'c']);
    }

    public function testPunctuationWithWhitespaceSplit(){
        $s = new Shlex('a  && b  ||  c', null, false, '&');

        $list = [];

        foreach ($s as $v){
            $list[] = $v;
        }

        $this->assertEqual($list, ['a', '&&', 'b', '|', '|', 'c']);

        $s = new Shlex('a  && b  ||  c', null, false, '&');
        $s->whitespaceSplit = true;

        $list = [];

        foreach ($s as $v){
            $list[] = $v;
        }

        $this->assertEqual($list, ['a', '&&', 'b', '||', 'c']);
    }

    public function testPunctuationWithPosix(){
        $s = new Shlex('f >"abc"', null, true, true);

        $list = [];

        foreach ($s as $v){
            $list[] = $v;
        }

        $this->assertEqual($list, ['f', '>', 'abc']);

        $s = new Shlex('f >\\"abc\\"', null, true, true);

        $list = [];

        foreach ($s as $v){
            $list[] = $v;
        }

        $this->assertEqual($list, ['f', '>', '"abc"']);
    }

    public function testEmptyStringHandling(){
        $expected = ['', ')', 'abc'];

        foreach ([false, true] as $punct){
            $s = new Shlex("'')abc", null, true, $punct);

            $slist = [];

            foreach ($s as $v){
                $slist[] = $v;
            }

            $this->assertEqual($slist, $expected);
        }

        $expected = ["''", ')', 'abc'];
        $s = new Shlex("'')abc", null, false, true);

        $list = [];

        foreach ($s as $v){
            $list[] = $v;
        }

        $this->assertEqual($list, $expected);
    }

    public function testQuote(){
        $safeunquoted = self::ASCII_LETTERS . self::DIGITS . '@%_-+=:,./';
        $unicode_sample = "\xe9\xe0\xdf";
        $unicode_sample = mb_convert_encoding($unicode_sample, 'UTF-8', 'ASCII');
        $unsafe = '"`$\\!' . $unicode_sample;

        $this->assertEqual(shlex_quote(''), "''");
        $this->assertEqual(shlex_quote($safeunquoted), $safeunquoted);
        $this->assertEqual(shlex_quote('test file name'), "'test file name'");

        function checkChar($str, $obj, $msg1, $msg2){
            $len = strlen($str);

            $start = 0;

            do{
                $charLen = 1;

                str_loop:
                $char = mb_strcut($str, $start, $charLen, "UTF-8");

                if($char === ''){
                    $charLen++;
                    goto str_loop;
                }

                $obj->assertEqual(shlex_quote(sprintf($msg1, $char)), sprintf($msg2, $char));

                $start += $charLen;
            }while($start < ($len - 1));
        }

        checkChar($unsafe, $this, "test%sname", "'test%sname'");
        checkChar($unsafe, $this, "test%s'name'", "'test%s'\"'\"'name'\"'\"''");
    }
}