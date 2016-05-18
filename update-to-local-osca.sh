FROM=$(realpath .)
TO=$(realpath $OSCA)
echo "from: $FROM"
echo "  to: $TO"
rsync -vrt --delete --exclude "*/.*" --exclude "*/.*/" --exclude=Debug $FROM $TO

