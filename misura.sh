#!/bin/bash

outsupervisor=$1
outclient=$2

declare -A secretClient 

# l'opzione '-d' segnale la fine della riga, settando il delimitatore del carattere a delim
while read line; do
	STR1=($line)
	case $line in
		*"SECRET "*)
		secretClient[${STR1[1]}]=${STR1[3]}
	esac
done < $outclient

declare -A stime 

while read line; do
	STR2=($line)
	case $line in
		*"BASED "*)
			stime[${STR2[4]}]=${STR2[2]} ;;
	esac
done < $outsupervisor

nstime=0
nstimeCorrette=0
nstimeErrate=0
totErrori=0
erroreMedio=0

for id in "${!secretClient[@]}"; do 
	diff=$(( stime[$id] - secretClient[$id] ))
	if (( $diff < 0 )); then
		diff=$((-$diff))
	fi
	if (( $diff <= 25 )); then 
		nstimeCorrette=$(( $nstimeCorrette + 1 ))
	fi
	if (( $diff > 25 )); then 
		nstimeErrate=$(( $nstimeErrate + 1 ))
	fi 
	totErrori=$(( $totErrori + $diff ))
done

nstime=$(($nstimeCorrette + $nstimeErrate))
erroreMedio=$(($totErrori/$nstime))

echo "Stime corrette: $nstimeCorrette / $nstime"
echo "Stime errate: $nstimeErrate / $nstime"
echo "Errore medio: $erroreMedio"
