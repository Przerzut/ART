/**
 * @file mainwindow.cpp
 * @brief Implementacja logiki głównego okna aplikacji Analizator Tramwajowy (ART).
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
 * @file mainwindow.cpp
 * @brief Implementacja logiki głównego okna aplikacji Analizator Tramwajowy (ART).
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
#include <QFile>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), isTracking(false), isPolish(false), startTime(0), currentTrackedId(-1) {

    loadRouteFromJson("1", ":/trasa1.geojson");
    lineFilterList = nullptr;

    m_tramModel = new TramModel(this);
    m_quickWidget = new QQuickWidget(this);
    
    m_quickWidget->engine()->rootContext()->setContextProperty("tramModel", m_tramModel);
    m_quickWidget->engine()->rootContext()->setContextProperty("mainWindow", this);
    
    m_quickWidget->setSource(QUrl("qrc:/map.qml"));
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    setupUI();
    setupCharts();        
    retranslateUi();

    networkManager = new QNetworkAccessManager(this);
    dataTimer = new QTimer(this);

    animTimer = new QTimer(this);
    connect(animTimer, &QTimer::timeout, this, &MainWindow::animateTrams);
    animTimer->start(33); 

    connect(btnToggle, &QPushButton::clicked, this, &MainWindow::toggleTracking);
    connect(btnLang, &QPushButton::clicked, this, &MainWindow::toggleLanguage);
    connect(dataTimer, &QTimer::timeout, this, &MainWindow::fetchTramData);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onResult);
    connect(tramIdComboBox, &QComboBox::currentTextChanged, this, &MainWindow::onTrackedTramChanged);

    updateVisibleRoutes();
}

void MainWindow::loadRouteFromJson(const QString& lineName, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Nie można otworzyć pliku trasy dla linii:" << lineName;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    QJsonArray features = root["features"].toArray();
    
    QVector<QPointF> currentRoute;
    for (const QJsonValue &feature : features) {
        QJsonObject geometry = feature.toObject()["geometry"].toObject();
        QString geomType = geometry["type"].toString();
        QJsonArray coords = geometry["coordinates"].toArray();
        
        // Jeśli tor jest zapisanym prostym odcinkiem
        if (geomType == "LineString") {
            for (const QJsonValue &coord : coords) {
                QJsonArray point = coord.toArray();
                currentRoute.append(QPointF(point[0].toDouble(), point[1].toDouble()));
            }
        } 
        // Jeśli tor jest pocięty na wiele połączonych kawałków (Standardowy eksport całej Linii)
        else if (geomType == "MultiLineString") {
            for (const QJsonValue &lineSegment : coords) {
                QJsonArray segmentCoords = lineSegment.toArray();
                for (const QJsonValue &coord : segmentCoords) {
                    QJsonArray point = coord.toArray();
                    currentRoute.append(QPointF(point[0].toDouble(), point[1].toDouble()));
                }
            }
        }
    }
    
    routes[lineName] = currentRoute; 
    qDebug() << "Załadowano punktów dla linii" << lineName << ":" << currentRoute.size();
}

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
    
    QStringList linie = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", 
    "11", "12", "13", "14", "15", "16", "17", "18", "19", "20", 
    "21", "22", "23", "24"
    };
    for (const QString& linia : linie) {
        QString filePath = QString(":/trasa%1.geojson").arg(linia);
        
        if (QFile::exists(filePath)) {
            loadRouteFromJson(linia, filePath);
            
            QListWidgetItem* item = new QListWidgetItem(linia, lineFilterList);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setCheckState(Qt::Checked);
        }
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

    QVBoxLayout *mapLayout = new QVBoxLayout(mapChartTab);
    mapLayout->setContentsMargins(0, 0, 0, 0); 
    mapLayout->addWidget(m_quickWidget);

    connect(lineFilterList, &QListWidget::itemChanged, this, &MainWindow::updateVisibleRoutes);
}

QVariantList MainWindow::routePaths() const {
    QVariantList activePaths;
    
    // Sprawdzamy wszystkie elementy na liście (odznaczane/zaznaczane ptaszkiem)
    if (lineFilterList) {
        for (int i = 0; i < lineFilterList->count(); ++i) {
            QListWidgetItem* item = lineFilterList->item(i);
            
            // Jeśli element jest zaznaczony (✔)
            if (item->checkState() == Qt::Checked) {
                QString lineName = item->text();
                
                // Wyciągamy jego trasę z naszej mapy, jeśli istnieje
                if (routes.contains(lineName)) {
                    QVariantList singlePath;
                    for (const QPointF& p : routes[lineName]) {
                        singlePath.append(QVariant::fromValue(QGeoCoordinate(p.y(), p.x())));
                    }
                    activePaths.append(QVariant::fromValue(singlePath));
                }
            }
        }
    }
    return activePaths; // Zwracamy TYLKO aktywne trasy do QML
}


void MainWindow::updateVisibleRoutes() {
    emit routePathsChanged(); 

    if (!lineFilterList || !m_tramModel) return;

    QStringList activeFilters;
    for (int i = 0; i < lineFilterList->count(); ++i) {
        if (lineFilterList->item(i)->checkState() == Qt::Checked) {
            activeFilters.append(lineFilterList->item(i)->text());
        }
    }

    QList<int> idsToRemove;
    for (auto it = tramLines.constBegin(); it != tramLines.constEnd(); ++it) {
        // Jeśli linia tego tramwaju nie znajduje się wśród aktywnych filtrów...
        if (!activeFilters.contains(it.value())) {
            idsToRemove.append(it.key()); // ...zapisujemy ID do usunięcia
        }
    }

    for (int id : idsToRemove) {
        m_tramModel->removeTram(id);       // Znika z mapy w QML
        targetAnimPositions.remove(id);    // Przestaje być animowany (cel)
        currentAnimPositions.remove(id);   // Przestaje być animowany (pozycja)
        previousPositions.remove(id);      // Czyścimy historię pozycji GPS
        speedBuffers.remove(id);           // Czyścimy bufor prędkości
        speedHistories.remove(id);         // Czyścimy historię wykresu prędkości
        tramLines.remove(id);              // Usuwamy z pamięci linii

        int idx = tramIdComboBox->findText(QString::number(id));
        if (idx != -1) tramIdComboBox->removeItem(idx);
        
        if (currentTrackedId == id) {
            tramIdComboBox->setCurrentIndex(0);
        }
    }
}

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
    speedLayout->setContentsMargins(0, 0, 0, 0); 
    speedLayout->addWidget(speedView);
}

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

void MainWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::LanguageChange) retranslateUi();
    QMainWindow::changeEvent(event);
}

void MainWindow::toggleLanguage() {
    isPolish = !isPolish; 
    if (isPolish) {
        if (appTranslator.load(":/app_pl.qm")) qApp->installTranslator(&appTranslator);
    } else {
        qApp->removeTranslator(&appTranslator);
    }
    retranslateUi(); 
}

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

QPointF MainWindow::snapToRoute(double lon, double lat, const QString& lineName) {
    if (!routes.contains(lineName) || routes[lineName].isEmpty()) {
        return QPointF(lon, lat); // Brak trasy w bazie -> rysuj surowy GPS
    }
    
    const QVector<QPointF>& routePoints = routes[lineName];
    QPointF bestPoint(lon, lat);
    double minDistanceSq = std::numeric_limits<double>::max();
    QPointF P(lon, lat);

    double cosLat = qCos(qDegreesToRadians(51.1079)); 

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
 * @details Opiera się na kinematycznym wektorze prędkości. W każdej klatce (33ms) 
 * przesuwa tramwaj zgodnie z jego wektorem, a następnie koryguje pozycję do krzywizny torów.
 */
void MainWindow::animateTrams() {
    if (!isTracking || targetAnimPositions.isEmpty()) return;

    double dt = 0.033; // Czas trwania jednej klatki (30 FPS)

    for (int id : targetAnimPositions.keys()) {
        QPointF target = targetAnimPositions[id];
        QPointF current = currentAnimPositions.value(id, target); 
        QPointF oldCurrent = current; // Zapisujemy pozycję przed ruchem do obliczenia kąta!

        QString myLine = tramLines.value(id, "");
        // 1. Obliczanie uśrednionej prędkości z bufora
        double speedKmh = 0.0;
        if (speedBuffers.contains(id) && !speedBuffers[id].isEmpty()) {
            for (double s : speedBuffers[id]) speedKmh += s;
            speedKmh /= speedBuffers[id].size();
        }

        // Jeśli tramwaj stoi w korku / na przystanku
        if (speedKmh < 1.0) {
            currentAnimPositions[id] = target;
            m_tramModel->updateTram(id, myLine, current.y(), current.x(), speedKmh, 0); 
            continue; 
        }

        // 2. Wyliczanie maksymalnego dystansu dla tej klatki animacji
        double speedMs = speedKmh / 3.6;
        double stepDegrees = (speedMs * dt) / 111320.0; 

        // 3. Budowa wektora kierunkowego (od current do target)
        double dx = target.x() - current.x();
        double dy = target.y() - current.y();
        double distanceToTarget = qSqrt(dx*dx + dy*dy);

        double moveX = 0;
        double moveY = 0;

        // SCENARIUSZ A: Jesteśmy w trasie, gonimy punkt docelowy z API
        if (distanceToTarget > stepDegrees) {
            moveX = (dx / distanceToTarget) * stepDegrees;
            moveY = (dy / distanceToTarget) * stepDegrees;
        } 
        // SCENARIUSZ B: API opóźnia się. Jedziemy w ciemno wzdłuż ostatniego wektora (Extrapolation)
        else {
            if (previousPositions.contains(id)) {
                QPointF prev = QPointF(previousPositions[id].lon, previousPositions[id].lat);
                double dirX = target.x() - prev.x();
                double dirY = target.y() - prev.y();
                double dirLen = qSqrt(dirX*dirX + dirY*dirY);

                if (dirLen > 0) {
                    // Łagodnie redukujemy prędkość w buforze (hamowanie przed potencjalnym przystankiem)
                    for (int i=0; i<speedBuffers[id].size(); ++i) {
                        speedBuffers[id][i] *= 0.98; 
                    }
                    if (speedKmh > 2.0) { // Przestajemy pchać, jeśli zwolnił do prędkości pieszego
                        moveX = (dirX / dirLen) * stepDegrees;
                        moveY = (dirY / dirLen) * stepDegrees;
                    }
                }
            }
        }

        QPointF newPos(current.x() + moveX, current.y() + moveY);

        current = snapToRoute(newPos.x(), newPos.y(), myLine);
        currentAnimPositions[id] = current;

        double actualDx = current.x() - oldCurrent.x();
        double actualDy = current.y() - oldCurrent.y();
        double heading = qRadiansToDegrees(qAtan2(actualDy, actualDx)) * -1 + 90;

        m_tramModel->updateTram(id, myLine, current.y(), current.x(), speedKmh, heading);
    }
}

void MainWindow::onTrackedTramChanged(const QString &text) {
    if (text == "AUTO") {
        setSelectedTramId(-1); 
    } else {
        setSelectedTramId(text.toInt()); 
    }
    
    speedSeries->clear();
    
    int id = selectedTramId();
    if (id != -1 && speedHistories.contains(id)) {
        speedSeries->replace(speedHistories[id]); 
    } 
    
    double currentSecs = (QDateTime::currentMSecsSinceEpoch() - startTime) / 1000.0;
    speedAxisX->setRange(qMax(0.0, currentSecs - 60.0), qMax(60.0, currentSecs));
    
    logConsole->append(tr("<i>[SYSTEM] Switched view to vehicle ID: %1</i>").arg(text));
}

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
        
        m_tramModel->removeTram(id); 

        if (currentTrackedId == id) {
            tramIdComboBox->setCurrentIndex(0); 
        }
    }
}

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

                tramLines[id] = lineName;

                double currentLat = obj["x"].toDouble(); 
                double currentLon = obj["y"].toDouble(); 
                
                QPointF snappedPos = snapToRoute(currentLon, currentLat, lineName);
                targetAnimPositions[id] = snappedPos; 
                if (!currentAnimPositions.contains(id)) currentAnimPositions[id] = snappedPos;

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

MainWindow::~MainWindow() {}