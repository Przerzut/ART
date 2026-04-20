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

/**
 * @brief Główna klasa aplikacji Analizator Ruchu Tramwajowego (ART).
 * Odpowiada za wyświetlanie interfejsu GUI oraz zarządzanie logiką pobierania
 * i parsowania danych sensorycznych (GPS) z API.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor klasy MainWindow.
     * @param parent Rodzic widżetu (domyślnie nullptr).
     */
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    /**
     * @brief Rozpoczyna lub zatrzymuje cykliczne pobieranie danych.
     */
    void toggleTracking();

    /**
     * @brief Inicjuje żądanie HTTP do miejskiego API w celu pobrania pozycji tramwajów.
     */
    void fetchTramData();

    /**
     * @brief Obsługuje odpowiedź z serwera i parsuje format JSON.
     * @param reply Odpowiedź sieciowa.
     */
    void onResult(QNetworkReply* reply);

private:
    void setupUI();
    void generateSimulatedData(); // Metoda ratunkowa (fallback) w razie awarii API

    // Moduły logiki
    QNetworkAccessManager* networkManager;
    QTimer* dataTimer;
    bool isTracking;

    // Elementy GUI
    QPushButton* btnToggle;
    QListWidget* lineFilterList;
    QTabWidget* mainTabs;
    QTextEdit* logConsole;      // Zakładka 1: Logi (w przyszłości Mapa)
    QWidget* speedChartTab;     // Zakładka 2: Miejsce na wykres prędkości
    QWidget* delayChartTab;     // Zakładka 3: Miejsce na wykres opóźnień
};

#endif // MAINWINDOW_H