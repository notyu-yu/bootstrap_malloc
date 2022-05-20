echo "build mdriver"
make
for fn in ./tracefiles/*.rep; do
	echo "=================================================="
	echo $fn
	./mdriver -a -f $fn
done
