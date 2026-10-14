// SDK headers first (they pull winsock2.h on Windows, which must precede windows.h)
#include "MfErrNo.h"
#include "Core.h"
#include "Sw_Device.h"
#include "Sw_ISO14443A-3.h"
#include "Sw_Mf_Classic.h"
#include "Tools.h"

#include "odalidbackend.h"
#include <QThread>
#include <cstring>

// ---- Card map (TP document, section 2.2) -----------------------------------
// Sector 2 : identity     - key A (read) / key B (write), index 2 in the reader's Secure Element
static constexpr uint8_t BLK_PRENOM   = 9;
static constexpr uint8_t BLK_NOM      = 10;
static constexpr uint8_t KEY_ID       = 2;
// Sector 3 : wallet       - key A (read, decrement) / key B (write, increment), index 3
static constexpr uint8_t BLK_BACKUP   = 13;
static constexpr uint8_t BLK_COUNTER  = 14;
static constexpr uint8_t KEY_WALLET   = 3;

struct OdalidBackend::Impl { ReaderName reader; };

OdalidBackend::OdalidBackend() : d(new Impl) { std::memset(&d->reader, 0, sizeof(ReaderName)); }
OdalidBackend::~OdalidBackend() { if (connected) disconnectReader(); delete d; }

// Record the SDK's own error text; re-poll the card so it is selectable again
// (after a failed authentication a MIFARE card falls back to the IDLE/HALT state).
bool OdalidBackend::fail(int16_t status, const char *what)
{
    m_error = QString("%1: %2 (%3)").arg(what, QString::fromLatin1(GetErrorMessage(status))).arg(status);
    uint8_t atq[2], sak[1], uid[12]; uint16_t len = 12;
    ISO14443_3_A_PollCard(&d->reader, atq, sak, uid, &len);
    return false;
}

// 2.3.1 Connect : GetLibraryExtension, OpenCOM, Version, RF_Power_Control(on)
bool OdalidBackend::connectReader()
{
    LIB_VERSION ext;
    GetLibraryExtension(&ext);

    std::memset(&d->reader, 0, sizeof(ReaderName));
    d->reader.Type   = ReaderCDC;
    d->reader.device = 0;

    int16_t st = OpenCOM(&d->reader);
    if (st != MI_OK) { m_error = QString("Reader not found: %1").arg(GetErrorMessage(st)); return false; }
    connected = true;

    st = Version(&d->reader);
    if (st != MI_OK) return fail(st, "Version");

    st = RF_Power_Control(&d->reader, TRUE, 0);
    if (st != MI_OK) return fail(st, "RF_Power_Control");

    LEDBuzzer(&d->reader, LED_GREEN_ON);          // idle = green
    return true;
}

// 2.3.6 Disconnect : RF_Power_Control(off), CloseCOM
bool OdalidBackend::disconnectReader()
{
    if (!connected) return true;
    LEDBuzzer(&d->reader, LED_OFF);
    int16_t st = RF_Power_Control(&d->reader, FALSE, 0);
    int16_t st2 = CloseCOM(&d->reader);
    connected = false;
    if (st != MI_OK)  { m_error = QString("RF_Power_Control: %1").arg(GetErrorMessage(st));  return false; }
    if (st2 != MI_OK) { m_error = QString("CloseCOM: %1").arg(GetErrorMessage(st2)); return false; }
    return true;
}

// 2.3.2 Select card : ISO14443_3_A_PollCard (ReqA + anticollision CL1 + select CL1)
bool OdalidBackend::selectCard(QString &uidHex)
{
    uint8_t atq[2], sak[1], uid[12]; uint16_t len = 12;
    int16_t st = ISO14443_3_A_PollCard(&d->reader, atq, sak, uid, &len);
    if (st != MI_OK) { m_error = "No card in the RF field"; return false; }
    uidHex.clear();
    for (int i = 0; i < len; ++i) uidHex += QString("%1").arg(uid[i], 2, 16, QChar('0')).toUpper();
    return true;
}

bool OdalidBackend::readBlock(uint8_t block, bool keyA, uint8_t keyIndex, QString &out)
{
    uint8_t data[17] = {0};                                   // 16 bytes + terminator
    int16_t st = Mf_Classic_Read_Block(&d->reader, TRUE, block, data, keyA ? AuthKeyA : AuthKeyB, keyIndex);
    if (st != MI_OK) return fail(st, "Mf_Classic_Read_Block");
    out = QString::fromLatin1(reinterpret_cast<char*>(data));  // zero padded -> stops at first 0
    return true;
}

bool OdalidBackend::writeBlock(uint8_t block, bool keyA, uint8_t keyIndex, const QString &in)
{
    uint8_t data[16] = {0};                                   // zero padded, max 16 chars
    const QByteArray b = in.toLatin1();
    std::memcpy(data, b.constData(), qMin<int>(16, b.size()));
    int16_t st = Mf_Classic_Write_Block(&d->reader, TRUE, block, data, keyA ? AuthKeyA : AuthKeyB, keyIndex);
    return st == MI_OK ? true : fail(st, "Mf_Classic_Write_Block");
}

// Identity is READ with key A (index 2)
bool OdalidBackend::readIdentity(QString &nom, QString &prenom)
{
    return readBlock(BLK_NOM,    true, KEY_ID, nom)
        && readBlock(BLK_PRENOM, true, KEY_ID, prenom);
}

// 2.3.3 Update : identity is WRITTEN with key B (index 2); the GUI then reads it back
bool OdalidBackend::writeIdentity(const QString &nom, const QString &prenom)
{
    return writeBlock(BLK_NOM,    false, KEY_ID, nom)
        && writeBlock(BLK_PRENOM, false, KEY_ID, prenom);
}

// Counter is READ with key A (index 3)
bool OdalidBackend::readCounter(uint32_t &units)
{
    int16_t st = Mf_Classic_Read_Value(&d->reader, TRUE, BLK_COUNTER, &units, AuthKeyA, KEY_WALLET);
    return st == MI_OK ? true : fail(st, "Mf_Classic_Read_Value");
}

// 2.3.4 Pay (key A, index 3):
//   Decrement block 14 and TRANSFER the result into the backup block 13,
//   then Restore: load block 13 and TRANSFER it back into block 14.
//   If the card is pulled away between the two steps, block 13 still holds a valid value.
bool OdalidBackend::decrementCounter(uint32_t n)
{
    int16_t st = Mf_Classic_Decrement_Value(&d->reader, TRUE, BLK_COUNTER, n, BLK_BACKUP, AuthKeyA, KEY_WALLET);
    if (st != MI_OK) return fail(st, "Mf_Classic_Decrement_Value");
    st = Mf_Classic_Restore_Value(&d->reader, TRUE, BLK_BACKUP, BLK_COUNTER, AuthKeyA, KEY_WALLET);
    return st == MI_OK ? true : fail(st, "Mf_Classic_Restore_Value");
}

// 2.3.5 Charge (key B, index 3): same mechanism with Increment
bool OdalidBackend::incrementCounter(uint32_t n)
{
    int16_t st = Mf_Classic_Increment_Value(&d->reader, TRUE, BLK_COUNTER, n, BLK_BACKUP, AuthKeyB, KEY_WALLET);
    if (st != MI_OK) return fail(st, "Mf_Classic_Increment_Value");
    st = Mf_Classic_Restore_Value(&d->reader, TRUE, BLK_BACKUP, BLK_COUNTER, AuthKeyB, KEY_WALLET);
    return st == MI_OK ? true : fail(st, "Mf_Classic_Restore_Value");
}

// Visual + sound feedback of the reader after each transaction
void OdalidBackend::signal(bool success)
{
    if (!connected) return;
    if (success) { LEDBuzzer(&d->reader, LED_GREEN_ON | BUZZER_ON); QThread::msleep(100); }
    else         { LEDBuzzer(&d->reader, LED_RED_ON   | BUZZER_ON); QThread::msleep(400); }
    LEDBuzzer(&d->reader, LED_GREEN_ON);
}
