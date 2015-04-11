HOST=localhost:8088

echo&&date&&
#-- - - -- -- - ------- - - - - -- - - - --- -- 
echo " * static document"&&
curl -si $HOST/qa/q01.txt>cmp&&
diff -q cmp t01.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- -
echo " * if-modified-since"&&
curl -siH "if-modified-since: Fri, 10 Apr 15 07:58:44 GMT" $HOST/qa/q01.txt>cmp&&
diff -q cmp t07.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * dynamic document"&&
curl -si $HOST/?hello>cmp&&
diff -q cmp t02.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * resumable download"&&
curl -sir 1- $HOST/qa/q01.txt>cmp&&
diff -q cmp t05.cmp&&
rm cmp&&
#-- - - -- -- - ------- - - - - -- - - - --- -- 
echo " * cached document"&&
curl -si $HOST/>cmp&&
diff -q cmp t06.cmp&&
rm cmp&&
#-- - - -- -- - ------- - - - - -- - - - --- -- 
echo " * illegal path"&&
echo $'GET ../../etc HTTP/1.1\r\n\r\n'|nc localhost 8088>cmp&&
diff -q cmp t08.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * page not found"&&
curl -si $HOST/asdf.html>cmp&&
diff -q cmp t03.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * widget not found"&&
curl -si $HOST/?asdf>cmp&&
diff -q cmp t04.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- -
date&&echo

