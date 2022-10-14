# MULTIFLOW DEVICE DRIVER
### Autore: Matteo Chiacchia (0300177)


## SPECIFICA
Il progetto permette la creazione e l'installazione di un Linux Device Driver che permette di eseguire operazioni di lettura e scrittura.
Questo implementa flussi di dati ad alta e bassa priorità in cui è possibile specificare il tipo di operazioni da effettura
I dati vengono letti seguendo un ordine FIFO e una volta consumati vengono eliminati dal flusso. 
Il flusso HIGH PRIORITY offre operazioni di scrittura e lettura sincrone. Il flusso LOW PRIORITY, invece, offre operazioni di scrittura asincrone (basate su delayed work) ma comunque mantenendo la capacità di notificare l'utente in maniera sincrona. Le operazioni di lettura invece rimangono sincrone. Questo driver dà anche il supporto alla funzione *ioctl()* per permettere all'utente di cambiare il tipo di sessione utilizzata (livello di priorità e tipo di operazioni, bloccanti o non bloccanti). Inoltre l'utente può anche decidere il valore massimo di timeout che si può aspettare nel caso di operazioni bloccanti. 
Il Driver mantiene la possibilità di generare 128 possibili device ognuno avente un minor number compreso tra 0 e 127.
Si dà anche la possibilità di diabilitare il device file, in questo caso infatti le sessioni aperte rimangono tali ma se si cerca di aprirne un'altra viene notificato il fatto che il device è disabilitato.
Riassumendo l'utente, una volta aperta una sessione con un device file specificato tramite minor number (0-127) è in grado di:
* Scrivere sul device
* Leggere dal device
* Cambiare tipo di sessione (cambiare priorità, bloccaggio e valore del timeout)
* Vedere stato del device (# bytes in ogni flusso, # thread in attesa)
* Abilitare o disabilitare un device file in termini di un minor number specifico
* Chiudere la sessione

## RUNNING

#### Driver
```bash
# compile kernel module (in ./driver)
make all

#  install kernel module
sudo insmod multiflow_driver.ko

# remove kernel module
sudo rmmod multiflow_driver.ko

# clean objects
sudo rmmod multiflow_driver.ko

# remove device files
sudo rm /dev/multiflow_device*

```

È stato creato anche uno script bash **module.sh** che esegue queste operazioni. Si può, quindi, eseguire direttamente lo script seguente:
```bash
sh module.sh
```

#### User
```bash
# compile user code
make

# N.B: DEVICE_NAME = "multiflow_device"

# run user code
sudo ./user [Device_Path] Major_number Minor_number
```

* Come **Device_Path** inserire preferibilmente */dev/multiflow_device*
* Come **Major_number** eseguire il comando
```bash
sudo dmesg
```
* Come **Minor_number** inserire un numero compreso tra 0 e 127


