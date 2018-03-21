#file acriot.sh
 #   authors Gabriele Barreca e Luca Murgia
  #   Si dichiara che il contenuto di questo file e' in ogni sua parte opera
   #  originale dell' autore.

#! /bin/bash
tabs 5 #Valore del tab
var=(PUT_OK PUTFAIL UPDATE UPDFAIL GET_OK GETFAIL REMOVE REMFAIL LOCKOK LOCKFAIL CONNECT CURSIZE CUROBJ	)
varmax=(MAXOBJ MAXCONN MAXSIZE)
flag=(0,0,0,0,0,0,0,0,0) #Array di flag che mi indica quale valore del TimeStamp va stampato
params=("${var[0]} \t${var[1]}" "\t ${var[2]} \t${var[3]}" " \t${var[4]} \t ${var[5]}" " \t${var[6]} \t${var[7]}" "\t ${var[8]} \t${var[9]}" "\t ${var[10]}" "\t ${var[11]}" "\t ${var[12]}")
paramsmax=("${varmax[0]}" " \t ${varmax[1]}" " \t ${varmax[2]}")

#Inizializzazioni variabili
utilizzo="Per infromazioni sull'utilizzo digitare il comando --help"

for ((i=0; i<9; i++)); do #Azzero flag
	flag[$i]=0
done

#Primo controllo presenza di opzioni e file
if [ $# == 0 ]; then
	echo $utilizzo 1>&2
	exit 1
fi
#Controllo se il parametro passato è la richiesta di istruzioni
if [ $1 == "--help" -a $# == 1 ] ; then
	cat < HowToUse.txt
	exit 1
fi

for arg in "$@"; do #Controllo se gli argomenti passati sono opzioni o il nome del file. In quest'ultimo caso salvo il nome in 'percorso'
  shift
  if [ -f $arg -a "${arg##*.}" == "txt" ] ; then 
  	percorso=$arg
  	set -- "$@" "-t"
  else 
  	set -- "$@" "$arg"
  fi
done

if [ -z ${percorso} ]; then #Controllo se il file è stato effettivamente passato
echo "File non presente" >&2
exit 1
fi

while getopts ":pugrcsomt" opzione #Parsing delle opzioni, in base all'opzione letta setto il flag a 1
	do
	  case $opzione in
		p) flag[0]=1 ;;
		u) flag[1]=1 ;;
		g) flag[2]=1 ;;
		r) flag[3]=1 ;;
		c) flag[5]=1 ;;
		s) flag[6]=1 ;;
		o) flag[7]=1 ;;
		m) flag[8]=1 ;;
		t) if [ $# = 1 ] ; then # Se non vi sono opzioni ed ho solo il nome del file, setto tutti i flag ad 1
				for ((i=0; i<9; i++)); do
						flag[$i]=1
				done
				fi ;;
		*) echo "Invalid option" >&2 ;;
		esac
	done
shift $(($OPTIND - 1))

#In base ai flag = 1, concateno i "titoli" dei valori

TITLE="# Timestamps\t"
for ((i=0; i<9; i++)); do 
	if [ "${flag[$i]}" == "1" ]; then
			TITLE+=" ${params[$i]}"
	fi
done

if [ ${flag[4]} -eq 1  -a ${flag[8]} -eq 1 ]; then 
TITLE+=" ${paramsmax[0]} ${paramsmax[2]}"
fi


#Inizializzo
maxconn=0
maxsize=0
maxobj=0 

Max="" #Stringa vuota


echo -e $TITLE
while read line; do #Inizio a leggere il file,riga per riga, salvandola prima in un array
IFS=' ' read -a array <<< $line
PRINT="${array[0]}   ${array[1]}   " #Timestamp e trattino, sono i primi valori della riga da stampare

#Calcolo i massimi valori di Maxconn Maxobj Maxsize (opzione -m)
if [ ${array[15]} -gt $maxconn ]; then
	maxconn=${array[15]}
fi
if [ ${array[16]} -gt $maxsize ]; then
	maxsize=${array[16]}
fi
if [ ${array[14]} -gt $maxobj ]; then
	maxobj=${array[14]}
fi

#Le rimanenti opzioni
if [ ${flag[0]} -eq 1 ]; then
	PRINT+="\t \t ${array[2]} \t \t ${array[3]}" #Put
fi
if [ ${flag[1]} -eq 1 ]; then
	PRINT+="\t \t ${array[4]} \t \t ${array[5]}" #Update
fi
if [ ${flag[2]} -eq 1 ]; then
	PRINT+="\t \t ${array[6]} \t \t ${array[7]}" #Get
fi
if [ ${flag[3]} -eq 1 ]; then
	PRINT+="\t \t ${array[8]} \t \t ${array[9]}" #Remove
fi
if [ ${flag[4]} -eq 1 ]; then
	PRINT+="\t 	\t ${array[10]} \t \t ${array[11]}" #Lock
fi
if [ ${flag[5]} -eq 1 ]; then
	PRINT+="\t \t ${array[12]}" #Numero connessioni
fi
if [ ${flag[6]} -eq 1 ]; then
	PRINT+="\t \t ${array[13]}" #Size KB
fi
if [ ${flag[7]} -eq 1 ]; then
	PRINT+="\t  \t ${array[14]}" #Numero oggetti
fi

if [ ${flag[0]} -eq 0 -a ${flag[1]} -eq 0 -a ${flag[2]} -eq 0 -a ${flag[3]} -eq 0 -a ${flag[4]} -eq 0 -a ${flag[5]} -eq 0 -a ${flag[6]} -eq 0 -a ${flag[7]} -eq 0 ]; then 
: #Se tutti i flag sono a 0 vuol dire che non devo stampare solo l'ultimo TimeStamp
elif  [ ${flag[4]} -eq 0 ]; then #Se arrivo qui vuol dire che li devo stampare tutti 
	echo -e $PRINT
fi

done < $percorso
Max+="\t${maxobj} \t \t  \t${maxconn} \t \t  \t${maxsize}" #Salvo Maxconn Maxobj Maxsize

if [ ${flag[4]} -eq 1 ]; then #Vuol dire che devo stampare solo l'ultimo TimeStamp e concateno Max con l'ultimo TimeStamp
PRINT+="\t ${array[15]} \t \t ${array[16]}"
echo -e $PRINT
fi

if [ ${flag[4]} -eq 0  -a ${flag[8]} -eq 1 ]; then #Nel caso debba stampare tutti i TimeStamp, stampo solo alla fine Maxconn Maxobj Maxsize
echo -e " ${paramsmax[0]}  ${paramsmax[1]}  ${paramsmax[2]} \n" $Max
fi

