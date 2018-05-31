bin="network_sgx_example_client"
instances=80

runstring=""

for ((i=1;i<=($instances);i++))
do
#  runstring=$runstring ./$bin > /dev/null &
#  runstring=$runstring ./$bin | sed "s/^/[client$i] /" &
   screen -dmS "c$i" bash -c "./$bin; exec bash"
done

#setsid sh -c '$runstring'
#pgid=$!

read -n1 -r -p "Press any key to continue..." key

#pkill -P $$

killall screen
