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
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>

struct VehicleData {
    double lon;         
    double lat;         
    qint64 timestampMs; 
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void toggleTracking();
    void fetchTramData();
    void onResult(QNetworkReply* reply);
    void toggleLanguage(); 
    void animateTrams();
    void onTrackedTramChanged(const QString &text); 

private:
    void setupUI();
    void setupCharts(); 
    void retranslateUi();
    double calculateSpeed(double lon1, double lat1, double lon2, double lat2, qint64 timeDiffMs);
    void initHardcodedRoute();                
    QPointF snapToRoute(double lon, double lat); 
    void cleanUpStaleTrams(qint64 currentTime); ///< NOWE: Funkcja czyszcząca stare tramwaje

    QTranslator appTranslator;
    QNetworkAccessManager* networkManager;
    QTimer* dataTimer;
    bool isTracking;
    bool isPolish;
    
    qint64 startTime; ///< NOWE: Prawdziwy czas startu do osi X
    int currentTrackedId; 

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

    QChart *mapChart;
    QScatterSeries *mapSeries;
    QScatterSeries *selectedMapSeries; 
    QLineSeries *routeSeries; 
    QValueAxis *mapAxisX;
    QValueAxis *mapAxisY;
};

#endif // MAINWINDOW_H