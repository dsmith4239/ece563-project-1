echo "============================================================"

make testcase1

echo "============================================================"
echo "Compile complete, printing output"
echo "============================================================"

./bin/testcase1

echo "============================================================"
echo "Saving output as my_testcase1.out"
echo "============================================================"
./bin/testcase1 > my_testcase1.out

echo "============================================================"
echo "Comparing outputs:"
echo "============================================================"

diff my_testcase1.out testcases/testcase1.out

echo "============================================================"
echo "Output saved to my_testcase1.out"
echo "nano my_testcase1.out to view"
echo "============================================================"
