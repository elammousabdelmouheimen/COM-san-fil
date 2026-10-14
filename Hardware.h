/*
*
* Copyright (c) 2009-2021 ODALID - https://odalid.com
*
*/

/** \file
* Type definition
* \author Vincent T.
* \version 0.1
* \date 18 Novembre 2021
*
*/

#ifndef HARDWARE_H
#define HARDWARE_H

#include "TypeDefs.h"
#include "Commun_Device.h"

#define ReaderTCP               1                            /**< Parametre pour utiliser un lecteur TCP/IP => AccessControl / TCP1 / TCP2 et TCP3. */
#define ReaderCDC               2                            /**< Parametre pour utiliser un lecteur USB => CDC1 / CDC2 / CDC3. */
#define ReaderPCSC              3                            /**< Parametre pour utiliser un lecteur PC/SC. */

#define DataMem                 512                         /**< Taille memoire. */

#define MAX_WAIT_READ_TO_BIT    1                           /**< temps max d'attente pour lecture (en ms). */
#define MAX_WAIT_READ           3000                        /**< temps max d'attente pour lecture (en ms). */

#define BlockingEnabled         FALSE                       /**< Utilisation des sockets bloquantes. */


/**
* \struct ReaderName
* \brief Objet contenant les informations du Reader
*
*/

typedef struct
{
    uint8_t CommMode;
    uint8_t CommModeTemp;
    uint8_t SessionType;
    uint8_t SessionKey[32];
    uint8_t random_a[16];
    uint8_t random_b[16];
    uint8_t TI[4];
    uint32_t CurrentCmdCtr;
    uint8_t serial_number[9];
}DEVICE_INFO;

typedef struct {
    uint8_t Type;			/*!< [In] Type de peripherique => ReaderTCP ou ReaderCDC */
    uint8_t device;			/*!< [In/Out] Port COM du peripherique pour ReaderCDC => CDC1 / CDC2 / CDC3*/
    char IPReader[16];		/*!< [In] Adresse IP du peripherique pour ReaderTCP => AccesControl / TCP1 / TCP2 / TCP3*/
    uint16_t nPort;         /*!< [In] Port du serveur du périphérique pour ReaderTCP par default nPort=0*/
    SOCKET hSocket;         /*!< [Out] Information sur la socket */
    HANDLE hCOM;            /*!< [Out] Information sur le port COM */
    char version[40];       /*!< [Out] Version */
    uint8_t serial[11];     /*!< [Out] Numero de serie */
    char stack[30];         /*!< [Out] Pile logiciel */
    DEVICE_INFO device_info;
}ReaderName;

#endif /*HARDWARE_H*/
