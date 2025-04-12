N=10
BLUE='\e[34m'
YELLOW='\e[33m'
BLACK='\e[30m'

printf "$YELLOW"
echo "compiling with N=$N"
gcc -DN=$N echo-server.c -o echo-server-foreground
printf "starting server $BLACK\n"


setsid ./echo-server-foreground &
sleep 3
echo "this text is printed by echo-server and got from 'echo' command!" > /tmp/echo_fifo
cat test.txt > /tmp/echo_fifo
pkill -USR1 echo-server-for
sleep 1
printf "$YELLOW"
printf "waiting $N seconds for alarm to ring$BLACK\n"
sleep $N
printf "$YELLOW"
printf "done sleeping, now sending another message $BLACK\n"

echo "another message after sleeping" > /tmp/echo_fifo

printf "$YELLOW"
printf "now going to demonize the echo-server and do all over again $BLACK\n"

pkill -HUP echo-server-for
sleep 1
printf "$YELLOW"
printf "sending messages to server $BLACK\n"
echo "this text is printed by echo-server and got from 'echo' command!" > /tmp/echo_fifo
cat test.txt > /tmp/echo_fifo
pkill -USR1 echo-server-for
sleep 1

printf "$YELLOW"
printf "waiting $N seconds for alarm to ring$BLACK\n"
sleep $N
printf "$YELLOW"
printf "done sleeping, now sending another message $BLACK\n"

echo "another message after sleeping" > /tmp/echo_fifo

printf "$YELLOW"
printf "the end of test $BLACK\n"
pkill -TERM echo-server-for

sleep 1
printf "$YELLOW"
printf "now running server in console with side program\n"
printf "every line written in console is sent to echo_fifo\n"
printf "all lines are treated as one big message. Quiting client will also turn off the server$BLACK\n"

setsid ./echo-server-foreground &
sleep 3
pkill echo-server-for -2
gcc echo-client.c -o echo-client
./echo-client
