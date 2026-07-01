#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QTimer>
#include <QtMultimedia/QMediaPlayer>
#include <QtMultimedia/QAudioOutput>
#include "person.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNextWordButtonClicked();
    void updateRhymeWord();
    void showTeamInfo();

private:
    QPushButton *nextWordButton;
    QList<Person> persons;
    QList<QLabel*> photoLabels;
    QList<QLabel*> nameLabels;
    QList<QLabel*> wordLabels;
    QStringList rhymeWords;
    int currentIndex;
    int currentWordIndex;
    QTimer *rhymeTimer;
    bool rhymeRunning;
    QLabel *highlightedLabel;
    QLabel *currentRhymeWordLabel;
    QLabel *teamLabel;
    QPushButton *aboutButton;

    QMediaPlayer *outPlayer;
    QMediaPlayer *fireworksPlayer;
    QAudioOutput *outAudioOutput;
    QAudioOutput *fireworksAudioOutput;

    QVector<QString> loadRhymes(const QString &filePath);
    QList<Person> loadPhotos(const QString &folderPath);
    void displayPhotosInCircle(const QList<Person> &persons);
    QString selectRandomRhyme(const QVector<QString> &rhymes);
    QString findLongestWord(const QString &rhyme);
    void removeCurrentPerson();
    void animateRemoval(QLabel *label);
    void displayWinner();
    void rebuildCircle();
    void updateWordOverPhoto(int index, const QString &word);
    void highlightCurrentPerson();
    void clearAllWordLabels();
    void loadSounds();
    QString teamInfoText() const;
};

#endif // MAINWINDOW_H