FROM=$(realpath .)
TO=$(realpath $OSCA)
echo "from: $FROM"
echo "  to: $TO"
rsync -vrt --exclude "*/.*" --exclude "*/.*/" --exclude=Debug $FROM $TO

