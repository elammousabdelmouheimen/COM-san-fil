#include "mainwindow.h"
#include "simbackend.h"
#ifdef USE_ODALID
#include "odalidbackend.h"
#endif

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
#ifdef USE_ODALID
    m_card = std::make_unique<OdalidBackend>();
#else
    m_card = std::make_unique<SimBackend>();
#endif
    buildUi();
    setState(false, false);
}

void MainWindow::buildUi()
{
    setWindowTitle("ODALID Porte monnaie");
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *grid = new QGridLayout(central);

    // ---- Header --------------------------------------------------------
    auto *title = new QLabel("<b>Porte monnaie</b><br>Cette application montre le fonctionnement<br>"
                             "d'un porte monnaie électronique sur carte MIFARE Classic 1k");
    auto *logo = new QLabel("<span style='font-size:28pt; color:#1a8fc0;'><i>odalid</i></span>");
    logo->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    grid->addWidget(title, 0, 0, 1, 2);
    grid->addWidget(logo, 0, 2, 1, 2);

    // ---- Left column : big logo + connection buttons -----------------------
    auto *bigLogo = new QLabel("<span style='font-size:64pt; color:#2a7fa8;'><i>odalid</i></span>");
    bigLogo->setAlignment(Qt::AlignCenter);
    bigLogo->setMinimumSize(260, 220);
    grid->addWidget(bigLogo, 1, 0, 1, 2);

    m_btnConnect    = new QPushButton("Connect");
    m_btnDisconnect = new QPushButton("Disconnect");
    m_btnSelect     = new QPushButton("Selectionner la carte");
    grid->addWidget(m_btnConnect,    2, 0);
    grid->addWidget(m_btnDisconnect, 2, 1);
    grid->addWidget(m_btnSelect,     3, 0, 1, 2);

    auto *about = new QLabel("Porte Monnaie V1.02<br>Copyright ODALID 2012 "
                             "<a href='http://odalid.com'>http://odalid.com</a><br>"
                             "<a href='mailto:contact@odalid.com'>contact@odalid.com</a>");
    about->setOpenExternalLinks(true);
    about->setStyleSheet("font-size:8pt;");
    grid->addWidget(about, 4, 0, 1, 2);

    // ---- Right column : identity -------------------------------------------------
    m_gId = new QGroupBox("Indentité");
    auto *gl = new QGridLayout(m_gId);
    m_nom = new QLineEdit; m_prenom = new QLineEdit;
    m_nom->setMaxLength(16); m_prenom->setMaxLength(16);   // 1 block = 16 bytes
    m_btnUpdate = new QPushButton("Mise à jour");
    gl->addWidget(new QLabel("Nom"), 0, 0);    gl->addWidget(m_nom, 0, 1);
    gl->addWidget(new QLabel("Prénom"), 1, 0); gl->addWidget(m_prenom, 1, 1);
    gl->addWidget(m_btnUpdate, 0, 2, 2, 1);
    grid->addWidget(m_gId, 1, 2, 1, 2);

    // ---- Decrement ----------------------------------------------------------------
    m_gDec = new QGroupBox("Décrémenter le porte monnaie");
    auto *dl = new QGridLayout(m_gDec);
    m_units = new QLabel("0");
    m_units->setFrameShape(QFrame::Box);
    m_units->setAlignment(Qt::AlignCenter);
    m_units->setMinimumWidth(60);
    m_dec = new QSpinBox; m_dec->setRange(0, 1000000);
    m_btnPay = new QPushButton("Payer");
    dl->addWidget(new QLabel("Nombre d'unités"), 0, 0); dl->addWidget(m_units, 0, 1);
    dl->addWidget(new QLabel("Nombre d'unités à décrémenter"), 1, 0);
    dl->addWidget(m_dec, 1, 1); dl->addWidget(m_btnPay, 1, 2);
    grid->addWidget(m_gDec, 2, 2, 2, 2);

    // ---- Increment ----------------------------------------------------------------
    m_gInc = new QGroupBox("Incrémenter le porte monnaie");
    auto *il = new QGridLayout(m_gInc);
    m_inc = new QSpinBox; m_inc->setRange(0, 1000000);
    m_btnCharge = new QPushButton("Charger");
    il->addWidget(new QLabel("Nombre d'unités à incrémenter"), 0, 0);
    il->addWidget(m_inc, 0, 1); il->addWidget(m_btnCharge, 0, 2);
    grid->addWidget(m_gInc, 4, 2, 1, 2);

    m_btnQuit = new QPushButton("Quitter");
    grid->addWidget(m_btnQuit, 5, 3);

    statusBar()->showMessage("Ready");

    connect(m_btnConnect,    &QPushButton::clicked, this, &MainWindow::onConnect);
    connect(m_btnDisconnect, &QPushButton::clicked, this, &MainWindow::onDisconnect);
    connect(m_btnSelect,     &QPushButton::clicked, this, &MainWindow::onSelectCard);
    connect(m_btnUpdate,     &QPushButton::clicked, this, &MainWindow::onUpdateIdentity);
    connect(m_btnPay,        &QPushButton::clicked, this, &MainWindow::onPay);
    connect(m_btnCharge,     &QPushButton::clicked, this, &MainWindow::onCharge);
    connect(m_btnQuit,       &QPushButton::clicked, this, &QWidget::close);
}

void MainWindow::setState(bool connected, bool cardSelected)
{
    m_btnConnect->setEnabled(!connected);
    m_btnDisconnect->setEnabled(connected);
    m_btnSelect->setEnabled(connected);
    for (QWidget *w : {static_cast<QWidget*>(m_gId), static_cast<QWidget*>(m_gDec), static_cast<QWidget*>(m_gInc)})
        w->setEnabled(cardSelected);
}

bool MainWindow::report(bool ok, const QString &okMsg)
{
    m_card->signal(ok);                       // "visual and/or sound info at each transaction"
    if (ok) {
        statusBar()->showMessage(okMsg, 5000);
    } else {
        statusBar()->showMessage("Error: " + m_card->lastError());
        QMessageBox::warning(this, "ODALID", m_card->lastError());
    }
    return ok;
}

void MainWindow::refreshCounter()
{
    uint32_t v = 0;
    if (m_card->readCounter(v)) m_units->setText(QString::number(v));
    else report(false, QString());
}

// Sequence 2.3.1 - Connect
void MainWindow::onConnect()
{
    if (report(m_card->connectReader(), "Reader connected")) setState(true, false);
}

// Sequence 2.3.6 - Disconnect
void MainWindow::onDisconnect()
{
    report(m_card->disconnectReader(), "Reader disconnected");
    setState(false, false);
    m_nom->clear(); m_prenom->clear(); m_units->setText("0");
}

// Sequence 2.3.2 - Select card : PollCard, read identity (block 9/10), read counter (block 14)
void MainWindow::onSelectCard()
{
    QString uid, nom, prenom;
    if (!report(m_card->selectCard(uid), QString()) ) { setState(true, false); return; }
    if (!m_card->readIdentity(nom, prenom))           { report(false, QString()); setState(true, false); return; }
    m_nom->setText(nom);
    m_prenom->setText(prenom);
    setState(true, true);
    refreshCounter();
    statusBar()->showMessage("Card selected, UID = " + uid, 5000);
}

// Sequence 2.3.3 - Update : write name/first name then read them back
void MainWindow::onUpdateIdentity()
{
    if (!report(m_card->writeIdentity(m_nom->text(), m_prenom->text()), "Identity updated")) return;
    QString nom, prenom;
    if (m_card->readIdentity(nom, prenom)) { m_nom->setText(nom); m_prenom->setText(prenom); }
}

// Sequence 2.3.4 - Pay : Decrement (-> backup), Restore (backup -> counter), Read
void MainWindow::onPay()
{
    uint32_t n = static_cast<uint32_t>(m_dec->value());
    if (n == 0) { statusBar()->showMessage("Choose a number of units to debit", 4000); return; }
    uint32_t cur = m_units->text().toUInt();
    if (n > cur) {
        QMessageBox::warning(this, "ODALID", "Insufficient balance");
        m_card->signal(false);
        return;
    }
    if (report(m_card->decrementCounter(n), QString("%1 unit(s) debited").arg(n))) {
        refreshCounter();
        m_dec->setValue(0);
    }
}

// Sequence 2.3.5 - Charge : Increment (-> backup), Restore (backup -> counter), Read
void MainWindow::onCharge()
{
    uint32_t n = static_cast<uint32_t>(m_inc->value());
    if (n == 0) { statusBar()->showMessage("Choose a number of units to credit", 4000); return; }
    if (report(m_card->incrementCounter(n), QString("%1 unit(s) credited").arg(n))) {
        refreshCounter();
        m_inc->setValue(0);
    }
}
