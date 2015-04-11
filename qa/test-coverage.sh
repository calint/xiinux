HOST=localhost
PORT=8080
HTTP=http://localhost:8088


echo&&date&&
#-- - - -- -- - ------- - - - - -- - - - --- -- 
echo " * static document"&&
curl -si $HTTP/qa/q01.txt>cmp&&
diff -q cmp t01.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- -
echo " * if-modified-since"&&
curl -siH "if-modified-since: Fri, 10 Apr 15 07:58:44 GMT" $HTTP/qa/q01.txt>cmp&&
diff -q cmp t07.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * dynamic document"&&
curl -si $HTTP/?hello>cmp&&
diff -q cmp t02.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * resumable download"&&
curl -sir 1- $HTTP/qa/q01.txt>cmp&&
diff -q cmp t05.cmp&&
rm cmp&&
#-- - - -- -- - ------- - - - - -- - - - --- -- 
echo " * cached document"&&
curl -si $HTTP/>cmp&&
diff -q cmp t06.cmp&&
rm cmp&&
#-- - - -- -- - ------- - - - - -- - - - --- -- 
echo " * illegal path"&&
echo $'GET ../../etc HTTP/1.1\r\n\r\n'|nc localhost 8088>cmp&&
diff -q cmp t08.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * page not found"&&
curl -si $HTTP/asdf.html>cmp&&
diff -q cmp t03.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * widget not found"&&
curl -si $HTTP/?asdf>cmp&&
diff -q cmp t04.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * chained requests"&&
echo $'GET / HTTP/1.1\r\n\r\nGET / HTTP/1.1\r\n\r\n'|nc localhost 8088>cmp&&
diff -q cmp t09.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- -
date&&echo

