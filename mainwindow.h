#pragma once
#include <QMainWindow>
#include <memory>
#include "cardbackend.h"

class QLineEdit; class QSpinBox; class QPushButton; class QLabel; class QGroupBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onConnect();
    void onDisconnect();
    void onSelectCard();
    void onUpdateIdentity();
    void onPay();
    void onCharge();

private:
    void buildUi();
    void setState(bool connected, bool cardSelected);
    void refreshCounter();
    bool report(bool ok, const QString &okMsg);   // status bar + LED/buzzer + error box

    std::unique_ptr<CardBackend> m_card;

    QLineEdit *m_nom = nullptr, *m_prenom = nullptr;
    QLabel *m_units = nullptr;
    QSpinBox *m_dec = nullptr, *m_inc = nullptr;
    QPushButton *m_btnConnect = nullptr, *m_btnDisconnect = nullptr, *m_btnSelect = nullptr;
    QPushButton *m_btnUpdate = nullptr, *m_btnPay = nullptr, *m_btnCharge = nullptr, *m_btnQuit = nullptr;
    QGroupBox *m_gId = nullptr, *m_gDec = nullptr, *m_gInc = nullptr;
};
