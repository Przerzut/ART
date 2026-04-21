/**
 * @file main.cpp
 * @brief Punkt wejścia aplikacji Analizator Ruchu Tramwajowego (ART).
 * * Inicjalizuje pętlę zdarzeń Qt i wyświetla główne okno aplikacji.
 */

#include "mainwindow.h"
#include <QApplication>

/**
 * @brief Główna funkcja programu.
 * @param argc Liczba argumentów linii komend.
 * @param argv Tablica argumentów linii komend.
 * @return Kod wyjściowy aplikacji.
 */
int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}