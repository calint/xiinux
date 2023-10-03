HOST=localhost
PORT=8088
HTTP=http://$HOST:$PORT

echo&&date&&echo coverage tests on $HTTP&&
#-- - - -- -- - ------- - - - - -- - - - --- -- 
echo " * small file"&&
curl -s $HTTP/qa/q01.txt>cmp&&
diff -q cmp t01.cmp&&
rm cmp&&
#-- - - -- -- - ------- - - - - -- - - - --- -- 
echo " * larger file 16K"&&
curl -s $HTTP/qa/ipsum16k.txt>cmp&&
diff -q cmp ipsum16k.txt&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- -
echo " * if-modified-since"&&
HEADER=$(printf "If-modified-since:";curl -si $HTTP/qa/q01.txt|grep ^Last-Modified:|awk '{printf $1="";print $0}')
curl -siH"$HEADER" $HTTP/qa/q01.txt>cmp&&
diff -q cmp t07.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * dynamic document"&&
curl -s $HTTP/qa/hello>cmp&&
diff -q cmp t02.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * resumable download"&&
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
echo -n $'GET ../../etc HTTP/1.1\r\n\r\n'|nc -w1 $HOST $PORT>cmp&&
diff -q cmp t08.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * page not found"&&
curl -si $HTTP/asdf.html>cmp&&
diff -q cmp t03.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
#echo " * widget not found"&&
#curl -s $HTTP/?asdf>cmp&&
#diff -q cmp t04.cmp&&
#rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
# !!! not fully supported. breaks when request bigger than buffer
#echo " * chained get"&&
#nc -w1 $HOST $PORT<t09.in>cmp&&
#diff -q cmp t09.cmp&&
#rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * post"&&
curl -s --header "Content-Type:text/plain;charset=utf-8" --data "hello ᐖᐛツ" $HTTP/qa/typealine>cmp&&
diff -q cmp t10.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * bigger post >4K"&&
curl -s --header "Content-Type:text/plain;charset=utf-8" --data-binary @q02.txt $HTTP/qa/typealine>cmp&&
diff -q cmp t11.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * upload small file 17B"&&
curl -sq -XPUT --header "Content-Type:file" --data-binary @q01.txt $HTTP/upl>/dev/null&&
curl -s $HTTP/upload/upl>cmp&&
diff -q cmp q01.txt&&
rm cmp&&
rm ../upload/upl&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * upload bigger file 128K"&&
curl -sq -XPUT --header "Content-Type:file" --data-binary @files/far_side_dog_ok.jpg $HTTP/upl>/dev/null&&
curl -s $HTTP/upload/upl>cmp&&
diff -q cmp files/far_side_dog_ok.jpg&&
rm cmp&&
rm ../upload/upl&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * upload file with utf-8 name" &&
curl -sq -XPUT --header "Content-Type:file" --data-binary @"files/hello ᐖᐛツ.txt" $HTTP/hello%20%E1%90%96%E1%90%9B%E3%83%84.txt > /dev/null &&
curl -s $HTTP/upload/hello%20%E1%90%96%E1%90%9B%E3%83%84.txt > cmp &&
diff -q cmp "files/hello ᐖᐛツ.txt" &&
rm cmp &&
[[ -e "../upload/hello ᐖᐛツ.txt" ]] &&
rm "../upload/hello ᐖᐛツ.txt" &&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
# !!! not fully supported. breaks when request bigger than buffer
#echo " * chained upload"&&
#nc -w1 $HOST $PORT<t12.in>cmp&&
#diff -q cmp t12.cmp&&
#curl -s $HTTP/upload/upl>cmp&&
#diff -q cmp t13.cmp&&
#curl -s $HTTP/upload/upl2>cmp&&
#diff -q cmp t14.cmp&&
#rm cmp&&
#rm ../upload/upl&&
#rm ../upload/upl2&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * chunked small 12B"&&
curl -s $HTTP/qa/chunked>cmp&&
diff -q cmp t15.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * chunked many small to larger than chunk size >4K "&&
curl -s $HTTP/qa/chunkedbig>cmp&&
diff -q cmp t16.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * chunked reply data larger than chunk buffer >256K "&&
curl -s $HTTP/qa/chunkedbigger>cmp&&
gunzip -fk t17.cmp.gz &&
diff -q cmp t17.cmp&&
rm cmp t17.cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
#echo " [todo] * resumable upload "&&
#curl -s $HTTP/?chunkedbigger>cmp&&
#gunzip -fk t17.cmp.gz &&
#diff -q cmp t17.cmp&&
#rm cmp t17.cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * widget zero length content post"&&
curl -s \
    --header "Content-Type:text/plain;charset=utf-8" \
    --header "Cookie: i=20230926--2020-abcdef" \
    --data-binary "" $HTTP/qa/page >cmp&&
diff -q cmp t20.cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * widget 'counter' "&&
curl -sH"Cookie: i=20230926--2020-abcdef" $HTTP/qa/counter?a=1+2 >cmp&&
diff -q cmp t19_1.cmp&&
curl -sH"Cookie: i=20230926--2020-abcdef" $HTTP/qa/counter?a=3+4 >cmp&&
diff -q cmp t19_2.cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * abuse request >1K "&&
nc -w1 $HOST $PORT<t18.in>cmp&&
diff -q cmp t18.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- -
date&&echo
