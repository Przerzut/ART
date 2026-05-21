/**
 * @file mainwindow.h
 * @brief Plik nagłówkowy głównego okna aplikacji Analizator Tramwajowy (ART).
 * @author Kacper Rzeszut
 * @date 2026-05-21
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QListWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QComboBox> 
#include <QTranslator>
#include <QEvent>
#include <QMap>
#include <QVector>
#include <QList>
#include <QPointF>
#include <QVariantList>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>

QT_CHARTS_USE_NAMESPACE

#include "trammodel.h"

#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>

struct VehicleData {
    double lon;         
    double lat;         
    qint64 timestampMs; 
};

class MainWindow : public QMainWindow {
    Q_OBJECT

    Q_PROPERTY(int selectedTramId READ selectedTramId WRITE setSelectedTramId NOTIFY selectedTramIdChanged)
    Q_PROPERTY(QVariantList routePath READ routePath CONSTANT)

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    int selectedTramId() const { return currentTrackedId; }
    void setSelectedTramId(int id) {
        if (currentTrackedId != id) {
            currentTrackedId = id;
            emit selectedTramIdChanged(id);
        }
    }

    QVariantList routePath() const;

signals:
    void selectedTramIdChanged(int id);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void toggleTracking();
    void fetchTramData();
    void onResult(QNetworkReply* reply);
    void toggleLanguage(); 
    void animateTrams();
    void onTrackedTramChanged(const QString &text); 
    void loadRouteFromJson();


private:
    void setupUI();
    void setupCharts(); 
    void retranslateUi();
    double calculateSpeed(double lon1, double lat1, double lon2, double lat2, qint64 timeDiffMs);
    void initHardcodedRoute();                
    QPointF snapToRoute(double lon, double lat); 
    void cleanUpStaleTrams(qint64 currentTime);

    QTranslator appTranslator;
    QNetworkAccessManager* networkManager;
    QTimer* dataTimer;
    bool isTracking;
    bool isPolish;
    qint64 startTime;
    int currentTrackedId; 

    TramModel* m_tramModel;
    QQuickWidget* m_quickWidget;

    QMap<int, VehicleData> previousPositions;
    QMap<int, QVector<double>> speedBuffers;     
    QMap<int, QList<QPointF>> speedHistories;    

    QTimer* animTimer; 
    QMap<int, QPointF> currentAnimPositions; 
    QMap<int, QPointF> targetAnimPositions;  
    QVector<QPointF> routePoints; 

    QPushButton* btnToggle;
    QPushButton* btnLang;
    QLabel* statusLabel;
    QLabel* filterLabel;
    QListWidget* lineFilterList;
    QLabel* tramIdLabel;       
    QComboBox* tramIdComboBox; 
    QLabel* headerLabel;
    QTextEdit* logConsole;       
    QTabWidget* mainTabs;        
    QWidget* speedChartTab;      
    QWidget* mapChartTab;        

    QChart *speedChart;
    QLineSeries *speedSeries;
    QValueAxis *speedAxisX;
    QValueAxis *speedAxisY;
};

#endif // MAINWINDOW_H