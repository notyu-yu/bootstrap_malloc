echo "=================================================="
echo "make the program"
make
echo "=================================================="
echo "short tests"
./mdriver -f ./tracefiles/short1.rep
./mdriver -f ./tracefiles/short2.rep
echo "=================================================="
echo "default test"
./mdriver -f ./tracefiles/amptjp.rep
echo "=================================================="
echo "realloc tests"
./mdriver -f ./tracefiles/realloc.rep
./mdriver -f ./tracefiles/realloc2.rep
