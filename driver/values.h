/**
 * 
 * Header contenente le definizioni delle MACRO e
 * dei parametri del modulo kernel (presenti in /sys/module/multiflow_driver/parameters)
 * 
 * Author: Matteo Chiacchia (0300177) 
 * 
 * 
 * */

#ifndef SOAPROJECT_VALUES_H
#define SOAPROJECT_VALUES_H


#define EXPORT_SYMTAB
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/kprobes.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/version.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/tty.h>     
#include <linux/version.h> 
#include <linux/moduleparam.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include "structs.h"

#define MODNAME "MFLOW-DEV"
#define DEVICE_NAME "multiflow_device"

#define MINORS 128  // Il numero di minors che il driver può gestire è 128
#define FLOWS 2     // Numero di flussi gestiti da un singolo device

#define MAX_DEVICE_BYTES 1000000 //Numero max di bytes gestibili dal device file

#define LOW_PRIORITY 0
#define HIGH_PRIORITY 1

#define BLOCKING 0
#define NON_BLOCKING 1

// Device abilitato a un'apertura di una sessione o meno
#define DISABLED 0 
#define ENABLED 1

// Gestione delle operazioni in ioctl()
#define CHANGE_PRIORITY_IOCTL 3 
#define CHANGE_BLOCKING_IOCTL 4



static int enabled_device[MINORS];
module_param_array(enabled_device, int, NULL, 0660);
MODULE_PARM_DESC(enabled_device, "Module parameter is implemented in order to enable or disable " \
"the driver file, in terms of a specific minor number. If it is disabled, " \
"any attempt to open a session should fail (but already open sessions will be still managed).");

static int hp_bytes[MINORS];
module_param_array(hp_bytes, int, NULL, 0440);
MODULE_PARM_DESC(hp_bytes, "Number of bytes currently present in the high priority flow.");

static int lp_bytes[MINORS];
module_param_array(lp_bytes, int, NULL, 0440);
MODULE_PARM_DESC(lp_bytes, "Number of bytes currently present in the low priority flow.");

static int hp_threads[MINORS];
module_param_array(hp_threads, int, NULL, 0440);
MODULE_PARM_DESC(hp_threads, "Number of threads currently in waiting along the high priority flow.");

static int lp_threads[MINORS];
module_param_array(lp_threads, int, NULL, 0440);
MODULE_PARM_DESC(lp_threads, "Number of threads currently in waiting along the low priority flow.");





#endif //SOAPROJECT_VALUES_H
