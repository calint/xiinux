HOST=localhost:8088

echo&&date&&
#-- - - -- -- - ------- - - - - -- - - - --- -- 
echo " * static document"&&
curl -si $HOST/qa/q01.txt>cmp&&
diff cmp t01.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * dynamic document"&&
curl -si $HOST/?hello>cmp&&
diff cmp t02.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * resumable download"&&
curl -sir 1- $HOST/qa/q01.txt>cmp&&
diff cmp t05.cmp&&
rm cmp&&
#-- - - -- -- - ------- - - - - -- - - - --- -- 
echo " * cached document"&&
curl -si $HOST/>cmp&&
diff cmp t06.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * page not found"&&
curl -si $HOST/asdf.html>cmp&&
diff cmp t03.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
echo " * widget not found"&&
curl -si $HOST/?asdf>cmp&&
diff cmp t04.cmp&&
rm cmp&&
#--- - - - - ---  - - - - -- - -- - -- - - -- - 
date&&echo

