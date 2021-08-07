//
// Created by Kyle Cesar on 07/08/21.
//

#ifndef KERNEL_PCI_H
#define KERNEL_PCI_H

#define PCI_BUSMAX      255
#define PCI_SLOTMAX     31
#define PCI_FUNCMAC     7
#define PCI_REGMAX      255


#define PCIR_DEVVENDOR      0x00
#define PCIR_VENDOR         0x00
#define PCIR_DEVICE         0x02
#define PCIR_COMMAND        0x04
#define PCIR_STATUS         0x06
#define PCIR_REVID          0x08
#define PCIR_PROGIF         0x09
#define PCIR_SUBCLASS       0x0A
#define PCIR_SUBCLASS       0x0A
#define PCIR_CLASS          0x0B
#define PCIR_CACHELNSZ      0x0C
#define PCIR_LATTIMER       0x0D
#define PCIR_HEADERTYPE     0X0E
#define PCIM_MFDEV          0x80
#define PCIM_PCIR_BIST      0x0F


#define PCIM_CMD_PORTEN         0x0001
#define PCIM_CMD_MEMEN          0x0002
#define PCIM_CMD_BUSMASTEREN    0x0004
#define PCIM_CMD_MWRICEN        0x0010
#define PCIM_CMD_PERRESPEN      0x0040





#endif //KERNEL_PCI_H
