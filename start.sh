sleep 2
cd /card/gtk/
sleep 30
rm -f /test.out
./test >test.out 2>&1 3>&1
