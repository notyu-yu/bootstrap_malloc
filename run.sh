echo "=================================================="
echo "make the program"
make
echo "=================================================="
echo "short tests"
./mdriver -a -f ./tracefiles/short1.rep
./mdriver -a -f ./tracefiles/short2.rep
echo "=================================================="
echo "default test"
./mdriver -a -f ./tracefiles/amptjp.rep
echo "=================================================="
echo "realloc tests"
./mdriver -a -f ./tracefiles/realloc.rep
./mdriver -a -f ./tracefiles/realloc2.rep
