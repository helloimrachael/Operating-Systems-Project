#!/bin/bash

echo "Test starting"
./supervisor 8 &> supervisor.log &
z=$!
echo "Supervisor started and servers created"
sleep 2

for (( i = 0; i < 10; ++i )); do
    ./client 5 8 20 >> client.log &
    ./client 5 8 20 >> client.log &
    sleep 1
done

for (( i = 0; i < 6; ++i )); do
    kill -SIGINT $z
    echo "SIGINT sent"
    sleep 10
done

kill -SIGINT $z
kill -SIGINT $z

echo "Termination signal sent"
echo "Test terminated successfully"

echo "Test misura.sh starting"
bash ./misura.sh supervisor.log client.log
