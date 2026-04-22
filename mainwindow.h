/**
 * @file mainwindow.h
 * @brief Plik nagłówkowy głównego okna aplikacji ART (Analizator Ruchu Tramwajowego).
 * * Zawiera definicję klasy MainWindow, która zarządza interfejsem graficznym,
 * filtrowaniem linii oraz asynchronicznym pobieraniem danych sensorycznych GPS.
 * Projekt realizowany w ramach kursu "Wizualizacja Danych Sensorycznych".
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

/**
 * @class MainWindow
 * @brief Klasa okna głównego aplikacji.
 * * Klasa odpowiada za realizację scenariuszy monitorowania pojazdów MPK Wrocław,
 * obsługę mechanizmu sygnałów i slotów Qt oraz dynamiczną zmianę języka interfejsu.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    /**
     * @brief Konstruktor klasy MainWindow.
     * @param parent Wskaźnik na obiekt nadrzędny (domyślnie nullptr).
     */
    explicit MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Destruktor klasy MainWindow.
     * Zwalnia zasoby i czyści obiekty powiązane z klasą.
     */
    ~MainWindow();

private slots:
    /**
     * @brief Slot przełączający stan monitoringu (START/STOP).
     * Uruchamia lub zatrzymuje QTimer odpowiedzialny za cykliczne odpytywanie API.
     */
    void toggleTracking();

    /**
     * @brief Wykonuje asynchroniczne żądanie HTTP POST do serwera pozycji pojazdów.
     * Wykorzystuje QNetworkAccessManager do pobrania aktualnego stanu floty.
     */
    void fetchTramData();

    /**
     * @brief Slot wywoływany po zakończeniu operacji sieciowej.
     * Odpowiada za parsowanie danych JSON i wyświetlanie ich w konsoli logów.
     * @param reply Wskaźnik na odpowiedź sieciową.
     */
    void onResult(QNetworkReply* reply);

    /**
     * @brief Slot zmieniający wersję językową aplikacji (PL/EN).
     * Aktualizuje teksty wszystkich etykiet i przycisków w interfejsie.
     */
    void toggleLanguage(); 

private:
    /**
     * @brief Inicjalizuje graficzny interfejs użytkownika.
     * Tworzy layouty, przyciski, listę filtrów oraz konsolę logów zgodnie z makietą.
     */
    void setupUI();

    QNetworkAccessManager* networkManager; ///< Menedżer operacji sieciowych.
    QTimer* dataTimer;                     ///< Timer do cyklicznego odświeżania danych (interwał 10s).
    bool isTracking;                       ///< Flaga określająca, czy monitoring jest aktywny.
    bool isPolish;                         ///< Flaga określająca aktualnie wybrany język interfejsu.

    QPushButton* btnToggle;      ///< Przycisk START/STOP sterujący strumieniem danych.
    QPushButton* btnLang;        ///< Przycisk zmiany języka.
    QLabel* statusLabel;         ///< Etykieta statusu połączenia.
    QLabel* filterLabel;         ///< Nagłówek sekcji filtrowania.
    QLabel* headerLabel;         ///< Główny nagłówek monitora trasy.
    QListWidget* lineFilterList; ///< Lista linii z Checkboxami do filtrowania.
    QTabWidget* mainTabs;        ///< Kontener zakładek dla wykresów analitycznych.
    QTextEdit* logConsole;       ///< Konsola wyświetlająca surowe dane sensoryczne w czasie rzeczywistym.
    QWidget* speedChartTab;      ///< Zakładka przeznaczona na wykres prędkości.
    QWidget* delayChartTab;      ///< Zakładka przeznaczona na wykres opóźnień.
};

#endif // MAINWINDOW_H