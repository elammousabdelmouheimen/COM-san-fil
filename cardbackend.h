#pragma once
#include <QString>
#include <cstdint>

// Abstract access to the reader + MIFARE Classic card.
// Every method returns true on success; on failure lastError() explains why.
class CardBackend
{
public:
    virtual ~CardBackend() = default;

    virtual bool connectReader()    = 0;   // OpenCOM + Version + RF_Power_Control(on)
    virtual bool disconnectReader() = 0;   // RF_Power_Control(off) + CloseCOM
    virtual bool selectCard(QString &uidHex) = 0;   // ISO14443_3_A_PollCard

    // Sector 2 : block 8 "Identite", 9 Prenom, 10 Nom (key A index 2 read / key B index 2 write)
    virtual bool readIdentity(QString &nom, QString &prenom) = 0;
    virtual bool writeIdentity(const QString &nom, const QString &prenom) = 0;

    // Sector 3 : block 12 "Porte Monnaie", 13 backup, 14 counter
    virtual bool readCounter(uint32_t &units) = 0;
    virtual bool decrementCounter(uint32_t n) = 0;   // Decrement -> backup, Restore backup -> counter
    virtual bool incrementCounter(uint32_t n) = 0;   // Increment -> backup, Restore backup -> counter

    // Visual / sound feedback of the reader after each transaction
    virtual void signal(bool success) = 0;

    QString lastError() const { return m_error; }

protected:
    QString m_error;
};
