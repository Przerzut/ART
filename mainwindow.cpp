/**
 * @file mainwindow.cpp
 * @brief Implementacja metod klasy MainWindow.
 * * Zawiera logikę obsługi interfejsu użytkownika, komunikacji sieciowej
 * z API MPK Wrocław oraz formatowania danych sensorycznych.
 */

#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QNetworkRequest>

/**
 * @brief Konstruktor okna głównego.
 * * Inicjalizuje interfejs, tworzy menedżera sieci oraz timery.
 * Łączy sygnały przycisków i mechanizmów sieciowych z odpowiednimi slotami.
 * @param parent Wskaźnik na obiekt nadrzędny.
 */
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), isTracking(false), isPolish(true) {
    setupUI();

    networkManager = new QNetworkAccessManager(this);
    dataTimer = new QTimer(this);

    connect(btnToggle, &QPushButton::clicked, this, &MainWindow::toggleTracking);
    connect(btnLang, &QPushButton::clicked, this, &MainWindow::toggleLanguage);
    connect(dataTimer, &QTimer::timeout, this, &MainWindow::fetchTramData);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onResult);
}

/**
 * @brief Inicjalizacja i rozmieszczenie elementów GUI.
 * * Tworzy układ boczny (filtry, przyciski) oraz panel główny (logi, zakładki).
 * Konfiguruje style CSS dla konsoli logów (kolory, czcionka monospace).
 */
void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    QVBoxLayout *sidePanelLayout = new QVBoxLayout();
    
    btnLang = new QPushButton("Zmień język (EN)", this);
    statusLabel = new QLabel("Status: Aktywny", this);
    
    btnToggle = new QPushButton("START", this);
    btnToggle->setMinimumHeight(45);
    btnToggle->setStyleSheet("background-color: #2e7d32; color: white; font-weight: bold;");

    filterLabel = new QLabel("<b>Filtrowanie linii:</b>", this);
    lineFilterList = new QListWidget(this);
    
    QStringList linie = {"1", "16", "145", "149"};
    for(const QString& linia : linie) {
        QListWidgetItem* item = new QListWidgetItem(linia, lineFilterList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(Qt::Checked);
    }

    sidePanelLayout->addWidget(btnLang);
    sidePanelLayout->addWidget(statusLabel);
    sidePanelLayout->addWidget(btnToggle);
    sidePanelLayout->addSpacing(10);
    sidePanelLayout->addWidget(filterLabel);
    sidePanelLayout->addWidget(lineFilterList);
    sidePanelLayout->addStretch();

    QVBoxLayout *rightPanelLayout = new QVBoxLayout();
    
    headerLabel = new QLabel("<b>MONITOR TRASY - DANE SENSORYCZNE LIVE</b>", this);
    
    logConsole = new QTextEdit(this);
    logConsole->setReadOnly(true);
    logConsole->setMinimumHeight(250);
    logConsole->setStyleSheet("background-color: #000000; color: #00FF00; font-family: 'Courier New';  white-space: pre;");

    mainTabs = new QTabWidget(this);
    speedChartTab = new QWidget(this);
    delayChartTab = new QWidget(this);
    
    mainTabs->addTab(speedChartTab, "Prędkość [km/h]");
    mainTabs->addTab(delayChartTab, "Opóźnienie [min]");

    rightPanelLayout->addWidget(headerLabel);
    rightPanelLayout->addWidget(logConsole);
    rightPanelLayout->addWidget(mainTabs);

    mainLayout->addLayout(sidePanelLayout, 1);
    mainLayout->addLayout(rightPanelLayout, 3);

    setCentralWidget(centralWidget);
    setWindowTitle("ART - Analizator Ruchu Tramwajowego");
    resize(1000, 750);
}

/**
 * @brief Zmienia wersję językową interfejsu (PL/EN).
 * * Dynamicznie aktualizuje teksty we wszystkich etykietach, przyciskach
 * oraz zakładkach obiektu QTabWidget.
 */
void MainWindow::toggleLanguage() {
    isPolish = !isPolish; 

    if (isPolish) {
        btnLang->setText(tr("Zmień język (EN)"));
        btnToggle->setText(isTracking ? tr("STOP") : tr("START"));
        statusLabel->setText(tr("Status: Aktywny"));
        headerLabel->setText(tr("<b>MONITOR TRASY - DANE SENSORYCZNE LIVE</b>"));
        
        filterLabel->setText(tr("<b>Filtrowanie linii:</b>"));
        mainTabs->setTabText(0, tr("Prędkość [km/h]"));
        mainTabs->setTabText(1, tr("Opóźnienie [min]"));
        
    } else {
        btnLang->setText(tr("Change Language (PL)"));
        btnToggle->setText(isTracking ? tr("STOP") : tr("START"));
        statusLabel->setText(tr("Status: Active"));
        headerLabel->setText(tr("<b>ROUTE MONITOR - LIVE SENSORY DATA</b>"));
        
        filterLabel->setText(tr("<b>Line Filtering:</b>"));
        mainTabs->setTabText(0, tr("Speed [km/h]"));
        mainTabs->setTabText(1, tr("Delay [min]"));
    }
}
/**
 * @brief Kontroluje stan śledzenia danych.
 * * Uruchamia lub zatrzymuje QTimer. Przy starcie wypisuje sformatowany
 * nagłówek tabeli do konsoli logów.
 */
void MainWindow::toggleTracking() {
    if(!isTracking) {
        dataTimer->start(10000); // 10 sekundowy interwał
        btnToggle->setText("STOP");
        btnToggle->setStyleSheet("background-color: #c62828; color: white; font-weight: bold; ");
        
        QString header = QString("<b>%1 | %2 | %3 | %4 | %5</b>")
                       .arg(isPolish ? "CZAS" : "TIME", -10)
                       .arg(isPolish ? "LINIA" : "LINE", -12)
                       .arg("ID", -14)
                       .arg(isPolish ? "POZ_X" : "POS_X", -11)
                       .arg(isPolish ? "POZ_Y" : "POS_Y", -10);
        
        header.replace(" ", "&nbsp;");
        logConsole->append(header);
        logConsole->append(QString(70, '-')); 
    
        fetchTramData();
    } else {
        dataTimer->stop();
        btnToggle->setText("START");
        btnToggle->setStyleSheet("background-color: #2e7d32; color: white; font-weight: bold;");
        logConsole->append(isPolish ? "<i>[SYSTEM] Monitoring zatrzymany.</i>" : "<i>[SYSTEM] Monitoring stopped.</i>");
    }
    isTracking = !isTracking;
}

/**
 * @brief Wysyła żądanie HTTP POST do serwera MPK.
 * * Przygotowuje parametry busList dla wybranych linii autobusowych i tramwajowych.
 */
void MainWindow::fetchTramData() {
    QUrl url("https://mpk.wroc.pl/bus_position");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    QByteArray postData = "busList[bus][]=1&busList[bus][]=16&busList[bus][]=145&busList[bus][]=149";
    networkManager->post(request, postData);
}

/**
 * @brief Przetwarza otrzymane dane JSON.
 * * Sprawdza błędy sieciowe, parsuje tablicę pojazdów i filtruje je
 * zgodnie z ustawieniami użytkownika w QListWidget.
 * @param reply Obiekt odpowiedzi sieciowej.
 */
void MainWindow::onResult(QNetworkReply* reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
        if(jsonDoc.isArray()) {
            QJsonArray records = jsonDoc.array();
            
            // Pobranie listy aktywnych filtrów
            QStringList activeFilters;
            for(int i = 0; i < lineFilterList->count(); ++i) {
                if(lineFilterList->item(i)->checkState() == Qt::Checked)
                    activeFilters.append(lineFilterList->item(i)->text());
            }

            for (const QJsonValue& val : records) {
                QJsonObject obj = val.toObject();
                QString lineName = obj["name"].toString();
                
                if(activeFilters.contains(lineName)) {
                    QString log = QString("%1 | %2 | ID: %3 | X: %4 | Y: %5")
                        .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), -10)
                        .arg("Linia " + lineName, -12)
                        .arg(QString::number(obj["k"].toInt()), -10)
                        .arg(obj["x"].toDouble(), 8, 'f', 4)
                        .arg(obj["y"].toDouble(), 8, 'f', 4);
                    logConsole->append(log);
                }
            }
        }
    }
    reply->deleteLater();
}

/**
 * @brief Destruktor klasy.
 */
MainWindow::~MainWindow() {}