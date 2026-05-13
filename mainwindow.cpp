/**
 * @file mainwindow.cpp
 * @brief Implementacja logiki głównego okna aplikacji Analizator Tramwajowy (ART).
 * @details Zawiera definicje metod odpowiedzialnych za komunikację z API MPK, 
 * przetwarzanie danych GPS, obliczanie prędkości oraz wizualizację na mapie i wykresach.
 */

#include "mainwindow.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QNetworkRequest>
#include <QApplication>
#include <QtMath>
#include <QSet>
#include <limits>

/**
 * @brief Konstruktor klasy MainWindow.
 * @details Inicjalizuje interfejs użytkownika, trasę, wykresy oraz menedżery sieci i timerów.
 * @param parent Wskaźnik na obiekt nadrzędny (domyślnie nullptr).
 */
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), isTracking(false), isPolish(false), startTime(0), currentTrackedId(-1) {
    setupUI();
    initHardcodedRoute(); 
    setupCharts();        
    retranslateUi();

    networkManager = new QNetworkAccessManager(this);
    dataTimer = new QTimer(this);

    animTimer = new QTimer(this);
    connect(animTimer, &QTimer::timeout, this, &MainWindow::animateTrams);
    animTimer->start(33); // Ok. 30 klatek na sekundę dla płynnej animacji

    connect(btnToggle, &QPushButton::clicked, this, &MainWindow::toggleTracking);
    connect(btnLang, &QPushButton::clicked, this, &MainWindow::toggleLanguage);
    connect(dataTimer, &QTimer::timeout, this, &MainWindow::fetchTramData);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onResult);
    connect(tramIdComboBox, &QComboBox::currentTextChanged, this, &MainWindow::onTrackedTramChanged);
}

/**
 * @brief Tworzy i układa widżety w głównym oknie.
 * @details Buduje boczny panel sterowania oraz główny panel z konsolą logów i zakładkami wykresów.
 */
void MainWindow::setupUI() {
    QWidget *centralWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);

    QVBoxLayout *sidePanelLayout = new QVBoxLayout();
    btnLang = new QPushButton(this);
    statusLabel = new QLabel(this);
    
    btnToggle = new QPushButton(this);
    btnToggle->setMinimumHeight(45);
    btnToggle->setStyleSheet("background-color: #2e7d32; color: white; font-weight: bold;");

    filterLabel = new QLabel(this);
    lineFilterList = new QListWidget(this);
    
    QStringList linie = {"16"}; 
    for(const QString& linia : linie) {
        QListWidgetItem* item = new QListWidgetItem(linia, lineFilterList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(Qt::Checked);
    }

    tramIdLabel = new QLabel(this);
    tramIdComboBox = new QComboBox(this);
    tramIdComboBox->addItem("AUTO");

    sidePanelLayout->addWidget(btnLang);
    sidePanelLayout->addWidget(statusLabel);
    sidePanelLayout->addWidget(btnToggle);
    sidePanelLayout->addSpacing(10);
    sidePanelLayout->addWidget(filterLabel);
    sidePanelLayout->addWidget(lineFilterList);
    sidePanelLayout->addSpacing(10);
    sidePanelLayout->addWidget(tramIdLabel);
    sidePanelLayout->addWidget(tramIdComboBox);
    sidePanelLayout->addStretch();

    QVBoxLayout *rightPanelLayout = new QVBoxLayout();
    headerLabel = new QLabel(this);
    
    logConsole = new QTextEdit(this);
    logConsole->setReadOnly(true);
    logConsole->setMaximumHeight(150);
    logConsole->setStyleSheet("background-color: #000000; color: #00FF00; font-family: 'Courier New'; white-space: pre;");

    mainTabs = new QTabWidget(this);
    speedChartTab = new QWidget(this);
    mapChartTab = new QWidget(this); 
    
    mainTabs->addTab(speedChartTab, "");
    mainTabs->addTab(mapChartTab, "");

    rightPanelLayout->addWidget(headerLabel);
    rightPanelLayout->addWidget(logConsole);
    rightPanelLayout->addWidget(mainTabs);

    mainLayout->addLayout(sidePanelLayout, 1);
    mainLayout->addLayout(rightPanelLayout, 3);

    setCentralWidget(centralWidget);
    resize(1050, 750);
}

/**
 * @brief Inicjalizuje punkty kontrolne trasy (Linii 16).
 * @details Definiuje łamaną torowiska, do której będą snapowane surowe pozycje GPS.
 */
void MainWindow::initHardcodedRoute() {
    routePoints.clear();
    routePoints.append(QPointF(17.0601, 51.0775));
    routePoints.append(QPointF(17.0465, 51.0921));
    routePoints.append(QPointF(17.0431, 51.0988));
    routePoints.append(QPointF(17.0490, 51.1060));
    routePoints.append(QPointF(17.0621, 51.1115));
    routePoints.append(QPointF(17.0590, 51.1160));
    routePoints.append(QPointF(17.0480, 51.1200));
    routePoints.append(QPointF(17.0270, 51.1230));
    routePoints.append(QPointF(17.0190, 51.1290));
    routePoints.append(QPointF(16.9950, 51.1390));
}

/**
 * @brief Konfiguruje system wykresów Qt Charts.
 * @details Tworzy serię danych dla prędkości, trasę torowiska oraz dwie serie punktów dla mapy 
 * (zwykłe oraz wybrany pojazd).
 */
void MainWindow::setupCharts() {
    speedSeries = new QLineSeries();
    QPen speedPen(Qt::blue); speedPen.setWidth(2); speedSeries->setPen(speedPen);

    speedChart = new QChart();
    speedChart->addSeries(speedSeries);
    speedChart->legend()->hide(); 
    
    speedAxisX = new QValueAxis(); speedAxisX->setRange(0, 60); speedAxisX->setLabelFormat("%d");
    speedChart->addAxis(speedAxisX, Qt::AlignBottom); speedSeries->attachAxis(speedAxisX);

    speedAxisY = new QValueAxis(); speedAxisY->setRange(0, 70); speedAxisY->setLabelFormat("%d");
    speedChart->addAxis(speedAxisY, Qt::AlignLeft); speedSeries->attachAxis(speedAxisY);

    QChartView *speedView = new QChartView(speedChart);
    speedView->setRenderHint(QPainter::Antialiasing);
    QVBoxLayout *speedLayout = new QVBoxLayout(speedChartTab);
    speedLayout->setContentsMargins(0, 0, 0, 0); speedLayout->addWidget(speedView);

    mapChart = new QChart(); mapChart->legend()->hide();

    routeSeries = new QLineSeries();
    QPen routePen(Qt::darkGray); routePen.setWidth(6); routeSeries->setPen(routePen);
    for (const QPointF& pt : routePoints) routeSeries->append(pt.x(), pt.y());
    mapChart->addSeries(routeSeries);

    mapSeries = new QScatterSeries();
    mapSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    mapSeries->setMarkerSize(12.0); mapSeries->setColor(Qt::red);
    mapChart->addSeries(mapSeries);

    selectedMapSeries = new QScatterSeries();
    selectedMapSeries->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    selectedMapSeries->setMarkerSize(18.0);
    selectedMapSeries->setColor(Qt::blue);
    mapChart->addSeries(selectedMapSeries);

    mapAxisX = new QValueAxis(); mapAxisX->setRange(16.98, 17.08); mapAxisX->setLabelFormat("%.3f");
    mapChart->addAxis(mapAxisX, Qt::AlignBottom);
    
    mapAxisY = new QValueAxis(); mapAxisY->setRange(51.07, 51.15); mapAxisY->setLabelFormat("%.3f");
    mapChart->addAxis(mapAxisY, Qt::AlignLeft);
    
    routeSeries->attachAxis(mapAxisX); routeSeries->attachAxis(mapAxisY);
    mapSeries->attachAxis(mapAxisX); mapSeries->attachAxis(mapAxisY);
    selectedMapSeries->attachAxis(mapAxisX); selectedMapSeries->attachAxis(mapAxisY);

    QChartView *mapView = new QChartView(mapChart);
    mapView->setRenderHint(QPainter::Antialiasing);
    QVBoxLayout *mapLayout = new QVBoxLayout(mapChartTab);
    mapLayout->setContentsMargins(0, 0, 0, 0); mapLayout->addWidget(mapView);
}

/**
 * @brief Aktualizuje teksty w interfejsie użytkownika.
 * @details Ustawia etykiety i nazwy zakładek korzystając z makra tr(), co pozwala na lokalizację językową.
 */
void MainWindow::retranslateUi() {
    btnLang->setText(tr("Change Language (PL)"));
    btnToggle->setText(isTracking ? tr("STOP") : tr("START"));
    statusLabel->setText(isTracking ? tr("Status: Active") : tr("Status: Inactive"));
    headerLabel->setText(tr("<b>ROUTE MONITOR - LIVE SENSORY DATA</b>"));
    filterLabel->setText(tr("<b>Line Filtering:</b>"));
    tramIdLabel->setText(tr("<b>Select Vehicle (ID):</b>"));
    mainTabs->setTabText(0, tr("Speed [km/h]"));
    mainTabs->setTabText(1, tr("Map 2D [GPS]"));
    setWindowTitle(tr("ART - Tram Traffic Analyzer"));
}

/**
 * @brief Obsługuje zdarzenia zmiany stanu aplikacji (np. język).
 * @param event Wskaźnik na obiekt zdarzenia.
 */
void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) retranslateUi();
    QMainWindow::changeEvent(event);
}

/**
 * @brief Przełącza język aplikacji między polskim a angielskim.
 * @details Ładuje odpowiedni plik .qm z zasobów i instaluje translator w aplikacji.
 */
void MainWindow::toggleLanguage() {
    isPolish = !isPolish; 
    if (isPolish) {
        if (appTranslator.load(":/app_pl.qm")) qApp->installTranslator(&appTranslator);
    } else {
        qApp->removeTranslator(&appTranslator);
    }
    retranslateUi(); 
}

/**
 * @brief Rozpoczyna lub zatrzymuje proces śledzenia danych.
 * @details Czyści logi, resetuje czas startu i aktywuje timer odpytywania sieci.
 */
void MainWindow::toggleTracking() {
    if(!isTracking) {
        startTime = QDateTime::currentMSecsSinceEpoch(); 
        dataTimer->start(10000);
        isTracking = true;
        retranslateUi();
        btnToggle->setStyleSheet("background-color: #c62828; color: white; font-weight: bold; ");
        
        QString header = QString("<b>%1 | %2 | %3 | %4 | %5</b>")
                       .arg(tr("TIME"), -10).arg(tr("LINE"), -12).arg("ID", -14).arg(tr("POS_X"), -11).arg(tr("POS_Y"), -10);
        header.replace(" ", "&nbsp;");
        logConsole->append(header);
        logConsole->append(QString(70, '-')); 
        
        fetchTramData();
    } else {
        dataTimer->stop();
        isTracking = false;
        retranslateUi();
        btnToggle->setStyleSheet("background-color: #2e7d32; color: white; font-weight: bold;");
        logConsole->append(tr("<i>[SYSTEM] Monitoring stopped.</i>"));
    }
}

/**
 * @brief Konstruuje i wysyła zapytanie HTTP POST do API MPK Wrocław.
 */
void MainWindow::fetchTramData() {
    QUrl url("https://mpk.wroc.pl/bus_position");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    QString postString = "";
    for(int i = 0; i < lineFilterList->count(); ++i) {
        if(lineFilterList->item(i)->checkState() == Qt::Checked) {
            postString += "busList[tram][]=" + lineFilterList->item(i)->text() + "&";
        }
    }
    if (postString.endsWith("&")) postString.chop(1);
    
    networkManager->post(request, postString.toUtf8());
}

/**
 * @brief Wykonuje proces "Map Matching" dla zadanych współrzędnych.
 * @details Rzutuje punkt GPS na najbliższy segment torowiska, uwzględniając korektę 
 * długości geograficznej (cosinus szerokości).
 * @param lon Długość geograficzna.
 * @param lat Szerokość geograficzna.
 * @return QPointF Punkt skorygowany do trasy.
 */
QPointF MainWindow::snapToRoute(double lon, double lat) {
    if (routePoints.isEmpty()) return QPointF(lon, lat);
    QPointF bestPoint(lon, lat);
    double minDistanceSq = std::numeric_limits<double>::max();
    QPointF P(lon, lat);

    double cosLat = qCos(qDegreesToRadians(lat)); 

    for (int i = 0; i < routePoints.size() - 1; ++i) {
        QPointF A = routePoints[i]; QPointF B = routePoints[i+1];
        
        double ABx = (B.x() - A.x()) * cosLat; 
        double ABy = B.y() - A.y();
        double APx = (P.x() - A.x()) * cosLat; 
        double APy = P.y() - A.y();
        
        double dotProduct = APx * ABx + APy * ABy;
        double segLengthSq = ABx * ABx + ABy * ABy;
        
        double t = (segLengthSq != 0) ? dotProduct / segLengthSq : -1;
        QPointF C = (t < 0) ? A : ((t > 1) ? B : QPointF(A.x() + t * (B.x() - A.x()), A.y() + t * (B.y() - A.y())));
        
        double distSq = pow((P.x() - C.x()) * cosLat, 2) + pow(P.y() - C.y(), 2);
        
        if (distSq < minDistanceSq) { minDistanceSq = distSq; bestPoint = C; }
    }
    return bestPoint;
}

/**
 * @brief Oblicza prędkość na podstawie dwóch punktów GPS (Wzór Haversine'a).
 * @param lon1 Długość punktu poprzedniego.
 * @param lat1 Szerokość punktu poprzedniego.
 * @param lon2 Długość punktu bieżącego.
 * @param lat2 Szerokość punktu bieżącego.
 * @param timeDiffMs Różnica czasu w milisekundach.
 * @return Prędkość w km/h.
 */
double MainWindow::calculateSpeed(double lon1, double lat1, double lon2, double lat2, qint64 timeDiffMs) {
    if (timeDiffMs <= 0) return 0.0;
    double R = 6371.0; 
    double dLat = qDegreesToRadians(lat2 - lat1); double dLon = qDegreesToRadians(lon2 - lon1);
    double a = qPow(qSin(dLat / 2), 2) + qCos(qDegreesToRadians(lat1)) * qCos(qDegreesToRadians(lat2)) * qPow(qSin(dLon / 2), 2);
    double c = 2 * qAtan2(qSqrt(a), qSqrt(1 - a));
    return (R * c) / (timeDiffMs / 1000.0 / 3600.0);
}

/**
 * @brief Realizuje płynną animację przemieszczania się tramwajów.
 * @details Funkcja wywoływana cyklicznie przez animTimer. Przesuwa kropki w stronę celu o 5% dystansu na klatkę.
 */
void MainWindow::animateTrams() {
    if (targetAnimPositions.isEmpty()) return;
    
    QList<QPointF> normalPoints;
    QList<QPointF> selectedPoints;
    bool needsUpdate = false;
    
    int renderTrackedId = currentTrackedId;
    if (renderTrackedId == -1 && !targetAnimPositions.isEmpty()) {
        renderTrackedId = targetAnimPositions.keys().first();
    }

    for (int id : targetAnimPositions.keys()) {
        QPointF target = targetAnimPositions[id];
        QPointF current = currentAnimPositions.value(id, target); 
        double dx = target.x() - current.x();
        double dy = target.y() - current.y();
        
        if (qAbs(dx) > 0.000001 || qAbs(dy) > 0.000001) {
            current.setX(current.x() + dx * 0.05); 
            current.setY(current.y() + dy * 0.05);
            needsUpdate = true;
        } else {
            current = target; 
        }
        currentAnimPositions[id] = current;
        
        if (id == renderTrackedId) selectedPoints.append(current);
        else normalPoints.append(current);
    }
    
    if(needsUpdate) {
        mapSeries->replace(normalPoints);
        selectedMapSeries->replace(selectedPoints);
    }
}

/**
 * @brief Obsługuje zmianę śledzonego tramwaju przez użytkownika.
 * @param text Tekst z ComboBoxa (ID pojazdu lub AUTO).
 */
void MainWindow::onTrackedTramChanged(const QString &text) {
    if (text == "AUTO") {
        currentTrackedId = -1;
    } else {
        currentTrackedId = text.toInt();
    }
    
    speedSeries->clear();
    
    if (currentTrackedId != -1 && speedHistories.contains(currentTrackedId)) {
        speedSeries->replace(speedHistories[currentTrackedId]); 
    } 
    
    double currentSecs = (QDateTime::currentMSecsSinceEpoch() - startTime) / 1000.0;
    speedAxisX->setRange(qMax(0.0, currentSecs - 60.0), qMax(60.0, currentSecs));
    
    logConsole->append(tr("<i>[SYSTEM] Switched view to vehicle ID: %1</i>").arg(text));
}

/**
 * @brief Usuwa pojazdy, które nie pojawiły się w raporcie API przez ponad 60 sekund.
 * @param currentTime Bieżący czas systemowy w ms.
 */
void MainWindow::cleanUpStaleTrams(qint64 currentTime) {
    QList<int> toRemove;
    for (auto it = previousPositions.constBegin(); it != previousPositions.constEnd(); ++it) {
        if (currentTime - it.value().timestampMs > 60000) { 
            toRemove.append(it.key());
        }
    }
    
    for (int id : toRemove) {
        int idx = tramIdComboBox->findText(QString::number(id));
        if (idx != -1) tramIdComboBox->removeItem(idx);
        
        previousPositions.remove(id);
        speedBuffers.remove(id);
        speedHistories.remove(id);
        currentAnimPositions.remove(id);
        targetAnimPositions.remove(id);
        
        if (currentTrackedId == id) {
            tramIdComboBox->setCurrentIndex(0); 
        }
    }
}

/**
 * @brief Slot obsługujący odpowiedź serwera z danymi o pozycjach.
 * @details Parsuje JSON, oblicza prędkości dla wszystkich pojazdów, loguje dane do konsoli 
 * i odświeża serie danych wykresów.
 * @param reply Wskaźnik na odpowiedź sieciową.
 */
void MainWindow::onResult(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        logConsole->append(tr("<i>[ERROR] API Failure: %1</i>").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
    if(jsonDoc.isArray()) {
        QJsonArray records = jsonDoc.array();
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        double currentSecs = (currentTime - startTime) / 1000.0;
        
        QStringList activeFilters;
        for(int i = 0; i < lineFilterList->count(); ++i) {
            if(lineFilterList->item(i)->checkState() == Qt::Checked) activeFilters.append(lineFilterList->item(i)->text());
        }

        int activeTargetId = -1;

        for (const QJsonValue& val : records) {
            QJsonObject obj = val.toObject();
            QString lineName = obj["name"].toString();
            
            if(activeFilters.contains(lineName)) {
                int id = obj["k"].toInt();
                
                double currentLat = obj["x"].toDouble(); 
                double currentLon = obj["y"].toDouble(); 
                
                QPointF snappedPos = snapToRoute(currentLon, currentLat);
                targetAnimPositions[id] = snappedPos; 

                QString log = QString("%1 | %2 | ID: %3 | X: %4 | Y: %5")
                    .arg(QDateTime::currentDateTime().toString("HH:mm:ss"), -10)
                    .arg(tr("Line ") + lineName, -12) 
                    .arg(QString::number(id), -10)
                    .arg(snappedPos.x(), 8, 'f', 4)
                    .arg(snappedPos.y(), 8, 'f', 4);
                logConsole->append(log);

                if (previousPositions.contains(id)) {
                    VehicleData prev = previousPositions[id];
                    if (prev.lon != currentLon || prev.lat != currentLat) {
                        double rawSpeed = calculateSpeed(prev.lon, prev.lat, currentLon, currentLat, currentTime - prev.timestampMs);
                        
                        if (rawSpeed <= 75.0) {
                            speedBuffers[id].append(rawSpeed);
                            if (speedBuffers[id].size() > 3) speedBuffers[id].removeFirst(); 

                            double smoothedSpeed = 0.0;
                            for (double s : speedBuffers[id]) smoothedSpeed += s;
                            smoothedSpeed /= speedBuffers[id].size(); 

                            speedHistories[id].append(QPointF(currentSecs, smoothedSpeed));
                            if (speedHistories[id].size() > 50) speedHistories[id].removeFirst();

                            if (id == currentTrackedId || (currentTrackedId == -1 && activeTargetId == -1)) {
                                speedSeries->append(currentSecs, smoothedSpeed);
                            }
                        }
                    }
                }
                previousPositions[id] = {currentLon, currentLat, currentTime};

                if (tramIdComboBox->findText(QString::number(id)) == -1) {
                    tramIdComboBox->addItem(QString::number(id));
                }
                
                if (activeTargetId == -1) activeTargetId = id; 
            }
        }

        cleanUpStaleTrams(currentTime); 
        
        speedAxisX->setRange(qMax(0.0, currentSecs - 60.0), qMax(60.0, currentSecs));
    }
    reply->deleteLater();
}

/**
 * @brief Destruktor okna głównego.
 */
MainWindow::~MainWindow() {}