git checkout .
sed -i "s/localhost: 127.0.0.1/localhost: 172.17.0.5/g" `grep "localhost: 127.0.0.1" -rl ./*.yml`
sed -i "s/p1: 127.0.0.1/p1: 172.17.0.6/g" `grep "p1: 127.0.0.1" -rl ./*.yml`
