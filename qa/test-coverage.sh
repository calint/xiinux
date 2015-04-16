HOST=localhost
PORT=8088
HTTP=http://localhost:8088


echo&&date&&
#-- - - -- -- - ------- - - - - -- - - - --- -- 
echo " * static document"&&
#curl -si $HTTP/qa/q01.txt>cmp&&
curl -s $HTTP/qa/q01.txt>cmp&&
diff -q cmp t01.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- -
echo " * if-modified-since"&&
HEADER=$(printf "If-modified-since:";curl -si $HTTP/qa/q01.txt|grep ^Last-Modified:|awk '{printf $1="";print $0}')
#echo $HEADER
curl -siH"$HEADER" $HTTP/qa/q01.txt>cmp&&
diff -q cmp t07.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * dynamic document"&&
curl -s $HTTP/?hello>cmp&&
diff -q cmp t02.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * resumable download"&&
#curl -sir 1- $HTTP/qa/q01.txt>cmp&&
curl -sr 1- $HTTP/qa/q01.txt>cmp&&
diff -q cmp t05.cmp&&
rm cmp&&
#-- - - -- -- - ------- - - - - -- - - - --- -- 
echo " * cached document"&&
curl -si $HTTP/>cmp&&
diff -q cmp t06.cmp&&
rm cmp&&
#-- - - -- -- - ------- - - - - -- - - - --- -- 
echo " * illegal path"&&
echo $'GET ../../etc HTTP/1.1\r\n\r\n'|nc $HOST $PORT>cmp&&
diff -q cmp t08.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * page not found"&&
curl -si $HTTP/asdf.html>cmp&&
diff -q cmp t03.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * widget not found"&&
curl -s $HTTP/?asdf>cmp&&
diff -q cmp t04.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * chained requests"&&
echo $'GET / HTTP/1.1\r\n\r\nGET / HTTP/1.1\r\n\r\n'|nc $HOST $PORT>cmp&&
diff -q cmp t09.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * post"&&
curl -s --header "Content-Type:text/plain;charset=utf-8" --data "hello ᐖᐛツ" $HTTP/?typealine>cmp&&
diff -q cmp t10.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * bigger post >4K"&&
curl -s --header "Content-Type:text/plain;charset=utf-8" --data-binary @q02.txt $HTTP/?typealine>cmp&&
diff -q cmp t11.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
#echo " * upload small file"&&
#curl -sq -XPUT --header "Content-Type:file" --data-binary @q01.txt $HTTP/upl>/dev/null&&
#curl -s $HTTP/upl>cmp&&
#diff -q cmp q01.txt&&
#rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * upload bigger file"&&
curl -sq -XPUT --header "Content-Type:file" --data-binary @q02.txt $HTTP/upl>/dev/null&&
curl -s $HTTP/upl>cmp&&
diff -q cmp q02.txt&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- -
date&&echo

