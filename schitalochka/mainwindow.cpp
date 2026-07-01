#include "mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QRandomGenerator>
#include <QPropertyAnimation>
#include <QRegularExpression>
#include <cmath>
#include <QMessageBox>
#include <QApplication>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , currentIndex(0)
    , currentWordIndex(0)
    , rhymeRunning(false)
    , highlightedLabel(nullptr)
    , teamLabel(nullptr)
    , aboutButton(nullptr)
    , outPlayer(nullptr)
    , fireworksPlayer(nullptr)
    , outAudioOutput(nullptr)
    , fireworksAudioOutput(nullptr)

{
    setFixedSize(1200, 800);
    setWindowTitle("Считалочка — командная версия");

    teamLabel = new QLabel(this);
    teamLabel->setText("Программа разработана: Севрук Андрей, Милякова Анна, Чопко Валерия");
    teamLabel->setAlignment(Qt::AlignCenter);
    teamLabel->setFont(QFont("Arial", 12, QFont::Bold));
    teamLabel->setStyleSheet("color: #1f2937; background-color: rgba(230, 245, 255, 220); padding: 8px; border: 1px solid #3b82f6; border-radius: 8px;");
    teamLabel->setGeometry(220, 10, 760, 40);
    teamLabel->show();

    aboutButton = new QPushButton("О проекте", this);
    aboutButton->setGeometry(width() - 140, 10, 120, 40);
    aboutButton->setFont(QFont("Arial", 10, QFont::Bold));
    aboutButton->setStyleSheet("background-color: #fef3c7; border: 1px solid #f59e0b; border-radius: 6px;");
    connect(aboutButton, &QPushButton::clicked, this, &MainWindow::showTeamInfo);

    nextWordButton = new QPushButton("Начать считалку", this);
    nextWordButton->setGeometry(width()/2 - 100, height() - 60, 200, 40);
    nextWordButton->setFont(QFont("Arial", 12, QFont::Bold));
    nextWordButton->setStyleSheet("background-color: lightblue; border-radius: 5px;");

    currentRhymeWordLabel = new QLabel(this);
    currentRhymeWordLabel->setAlignment(Qt::AlignCenter);
    currentRhymeWordLabel->setFont(QFont("Arial", 24, QFont::Bold));
    currentRhymeWordLabel->setStyleSheet("color: black; background-color: transparent; padding: 15px; border: 2px solid black; border-radius: 10px;");
    currentRhymeWordLabel->resize(300, 80);
    currentRhymeWordLabel->move(width()/2 - 150, height()/2 - 40);
    currentRhymeWordLabel->hide();

    connect(nextWordButton, &QPushButton::clicked, this, &MainWindow::onNextWordButtonClicked);

    QString appPath = QApplication::applicationDirPath();
    QVector<QString> rhymes = loadRhymes(appPath + "/texts/rhymes.txt");
    persons = loadPhotos(appPath + "/photos");

    if (persons.isEmpty()) {
        QMessageBox::critical(this, "Ошибка",
                              "Не найдены фотографии!\n\n"
                              "Создайте папку: " + appPath + "/photos/\n"
                                              "И добавьте туда фото (jpg, png)");
        return;
    }

    if (rhymes.isEmpty()) {
        QMessageBox::critical(this, "Ошибка",
                              "Не найдены считалки!\n\n"
                              "Создайте файл: " + appPath + "/texts/rhymes.txt\n"
                                              "Добавьте 10+ строк со считалками");
        return;
    }

    displayPhotosInCircle(persons);

    QString rhyme = selectRandomRhyme(rhymes);
    rhymeWords = rhyme.split(QRegularExpression("\\s+"));

    rhymeTimer = new QTimer(this);
    rhymeTimer->setSingleShot(false);
    connect(rhymeTimer, &QTimer::timeout, this, &MainWindow::updateRhymeWord);

    qDebug() << "Загружено" << persons.size() << "фотографий";
    qDebug() << "Считалка:" << rhyme;
    qDebug() << "Слов в считалке:" << rhymeWords.size();

    loadSounds();
}

MainWindow::~MainWindow()
{
    delete rhymeTimer;
    delete nextWordButton;
    delete aboutButton;
    delete teamLabel;
    delete outPlayer;
    delete fireworksPlayer;
    delete outAudioOutput;
    delete fireworksAudioOutput;
}

QString MainWindow::teamInfoText() const
{
    return "Проект: Считалочка\n"
           "Командная версия для учебной практики\n\n"
           "Команда: «Счётная палата»\n"
           "Участники команды:\n"
           "• Севрук Андрей Сергеевич — работа с интерфейсом и основной логикой приложения;\n"
           "• Милякова Анна — работа с ресурсами, считалками и оформлением проекта;\n"
           "• Чопко Валерия — тестирование, документация и подготовка проекта к GitHub.\n\n"
           "Программа размещает фотографии участников по кругу, выбирает случайную считалку "
           "и последовательно исключает участников до определения победителя.";
}

void MainWindow::showTeamInfo()
{
    QMessageBox::information(this, "О проекте", teamInfoText());
}

QVector<QString> MainWindow::loadRhymes(const QString &filePath)
{
    QVector<QString> rhymes;
    QFile file(filePath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                rhymes.append(line);
            }
        }
        file.close();
        qDebug() << "Загружено считалок:" << rhymes.size();
    } else {
        qDebug() << "Не удалось открыть файл:" << filePath;
    }

    return rhymes;
}

QList<Person> MainWindow::loadPhotos(const QString &folderPath)
{
    QList<Person> persons;
    QDir dir(folderPath);

    if (!dir.exists()) {
        qDebug() << "Папка не существует:" << folderPath;
        return persons;
    }

    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp";
    dir.setNameFilters(filters);

    QFileInfoList fileList = dir.entryInfoList();
    for (const QFileInfo &fileInfo : fileList) {
        QPixmap pixmap(fileInfo.absoluteFilePath());
        if (!pixmap.isNull()) {
            persons.append(Person(fileInfo.baseName(), pixmap));
        }
    }

    qDebug() << "Загружено фото:" << persons.size();
    return persons;
}

void MainWindow::highlightCurrentPerson()
{
    if (currentIndex >= 0 && currentIndex < photoLabels.size()) {
        highlightedLabel = photoLabels[currentIndex];
        if (highlightedLabel) {
            highlightedLabel->setStyleSheet("border: 4px solid red; border-radius: 5px;");
        }
    }
}
void MainWindow::loadSounds()
{
    QString appPath = QApplication::applicationDirPath();

    outPlayer = new QMediaPlayer(this);
    outAudioOutput = new QAudioOutput(this);
    outPlayer->setAudioOutput(outAudioOutput);
    outAudioOutput->setVolume(0.5);

    QString outPath = appPath + "/sounds/out.mp3";
    if (QFile::exists(outPath)) {
        outPlayer->setSource(QUrl::fromLocalFile(outPath));
        qDebug() << "Загружен звук вылетания:" << outPath;
    } else {
        qDebug() << "Звук не найден:" << outPath;
    }

    fireworksPlayer = new QMediaPlayer(this);
    fireworksAudioOutput = new QAudioOutput(this);
    fireworksPlayer->setAudioOutput(fireworksAudioOutput);
    fireworksAudioOutput->setVolume(0.5);

    QString fireworkPath = appPath + "/sounds/fireworks.mp3";
    if (QFile::exists(fireworkPath)) {
        fireworksPlayer->setSource(QUrl::fromLocalFile(fireworkPath));
        qDebug() << "Загружен звук фейерверков:" << fireworkPath;
    } else {
        qDebug() << "Звук не найден:" << fireworkPath;
    }
}
void MainWindow::displayPhotosInCircle(const QList<Person> &persons)
{
    int centerX = width() / 2;
    int centerY = height() / 2;

    int radiusX = 500;
    int radiusY = 250;

    int count = persons.size();
    int photoSize = 100;

    for (int i = 0; i < count; ++i) {
        double angle = (2 * M_PI / count) * i;

        int x = centerX + radiusX * cos(angle) - photoSize / 2;
        int y = centerY + radiusY * sin(angle) - photoSize / 2;

        QLabel *label = new QLabel(this);
        label->setPixmap(persons[i].photo.scaled(photoSize, photoSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        label->setFixedSize(photoSize, photoSize);
        label->move(x, y);
        label->setObjectName(persons[i].name);
        label->show();
        photoLabels.append(label);

        QLabel *nameLabel = new QLabel(persons[i].name, this);
        nameLabel->setFont(QFont("Arial", 9));
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setGeometry(x, y + photoSize, photoSize, 20);
        nameLabel->show();
        nameLabels.append(nameLabel);

        QLabel *wordLabel = new QLabel("", this);
        wordLabel->setFont(QFont("Arial", 10, QFont::Bold));
        wordLabel->setAlignment(Qt::AlignCenter);
        wordLabel->setGeometry(x, y - 25, photoSize, 20);
        wordLabel->hide();
        wordLabels.append(wordLabel);
    }
}
void MainWindow::updateWordOverPhoto(int index, const QString &word)
{
    for (QLabel *label : wordLabels) {
        label->setText("");
        label->hide();
    }

    if (index >= 0 && index < wordLabels.size()) {
        wordLabels[index]->setText(word);
        wordLabels[index]->show();
    }
}

void MainWindow::clearAllWordLabels()
{
    for (QLabel *label : wordLabels) {
        label->setText("");
        label->hide();
    }
}

QString MainWindow::selectRandomRhyme(const QVector<QString> &rhymes)
{
    if (rhymes.isEmpty()) return QString();
    int index = QRandomGenerator::global()->bounded(rhymes.size());
    return rhymes[index];
}

QString MainWindow::findLongestWord(const QString &rhyme)
{
    QStringList words = rhyme.split(QRegularExpression("\\s+"));
    QString longestWord;
    for (const QString &word : words) {
        if (word.length() > longestWord.length()) {
            longestWord = word;
        }
    }
    return longestWord;
}

void MainWindow::onNextWordButtonClicked()
{
    if (persons.isEmpty() || rhymeWords.isEmpty()) {
        return;
    }

    if (!rhymeRunning) {
        rhymeRunning = true;
        currentWordIndex = 0;
        currentIndex = QRandomGenerator::global()->bounded(persons.size());

        currentRhymeWordLabel->setText(rhymeWords[0]);
        currentRhymeWordLabel->show();

        updateWordOverPhoto(currentIndex, rhymeWords[0]);

        highlightCurrentPerson();

        rhymeTimer->start(700);

        nextWordButton->setEnabled(false);
        nextWordButton->setVisible(false);
    }
}

void MainWindow::updateRhymeWord()
{
    if (persons.isEmpty()) {
        rhymeTimer->stop();
        return;
    }

    if (highlightedLabel) {
        highlightedLabel->setStyleSheet("");
    }

    currentWordIndex++;

    if (currentWordIndex >= rhymeWords.size()) {
        rhymeTimer->stop();

        int indexToRemove = currentIndex;

        QTimer::singleShot(500, this, [this, indexToRemove]() {
            if (indexToRemove < persons.size()) {
                currentIndex = indexToRemove;
                removeCurrentPerson();
            }
            clearAllWordLabels();
        });
        return;
    }
 currentRhymeWordLabel->setText(rhymeWords[currentWordIndex]);
    currentIndex = (currentIndex + 1) % persons.size();

    updateWordOverPhoto(currentIndex, rhymeWords[currentWordIndex]);
     highlightCurrentPerson();

}


void MainWindow::removeCurrentPerson()
{
    if (currentIndex < 0 || currentIndex >= persons.size()) return;

    if (currentIndex < photoLabels.size()) {
        animateRemoval(photoLabels[currentIndex]);
        animateRemoval(nameLabels[currentIndex]);
        animateRemoval(wordLabels[currentIndex]);
    }

    persons.removeAt(currentIndex);
    photoLabels.removeAt(currentIndex);
    nameLabels.removeAt(currentIndex);
    wordLabels.removeAt(currentIndex);

    if (persons.size() == 1) {
        QTimer::singleShot(1000, this, &MainWindow::displayWinner);
    } else if (persons.size() > 0) {
        if (currentIndex >= persons.size()) {
            currentIndex = 0;
        }

        rebuildCircle();

        QTimer::singleShot(500, this, [this]() {
            currentWordIndex = 0;
            rhymeRunning = true;
            currentRhymeWordLabel->setText(rhymeWords[0]);
            currentRhymeWordLabel->show();
            updateWordOverPhoto(currentIndex, rhymeWords[0]);
            rhymeTimer->start(700);
        });
    }
}
void MainWindow::rebuildCircle()
{
    for (QLabel *label : photoLabels) {
        if (label) label->deleteLater();
    }
    for (QLabel *label : nameLabels) {
        if (label) label->deleteLater();
    }
    for (QLabel *label : wordLabels) {
        if (label) label->deleteLater();
    }

    photoLabels.clear();
    nameLabels.clear();
    wordLabels.clear();

    displayPhotosInCircle(persons);
}

void MainWindow::animateRemoval(QLabel *label)
{
    if (!label) return;

    if (outPlayer && outPlayer->source() != QUrl()) {
        outPlayer->play();
    }

    QPropertyAnimation *animation = new QPropertyAnimation(label, "geometry");
    animation->setDuration(800);
    animation->setStartValue(label->geometry());
    animation->setEndValue(QRect(width() - 150, -label->height(), label->width(), label->height()));
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::displayWinner()
{
    if (persons.size() != 1) return;

    if (fireworksPlayer && fireworksPlayer->source() != QUrl()) {
        fireworksPlayer->play();
    }

    for (QLabel *label : photoLabels) {
        if (label) label->hide();
    }
    for (QLabel *label : nameLabels) {
        if (label) label->hide();
    }
    for (QLabel *label : wordLabels) {
        if (label) label->hide();
    }
    if (currentRhymeWordLabel) {
        currentRhymeWordLabel->hide();
    }

    QLabel *winnerLabel = new QLabel(this);
    winnerLabel->setPixmap(persons[0].photo.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    winnerLabel->setFixedSize(300, 300);
    winnerLabel->move(width()/2 - 150, height()/2 - 150);
    winnerLabel->show();

    QLabel *nameWinner = new QLabel(persons[0].name, this);
    nameWinner->setFont(QFont("Arial", 20, QFont::Bold));
    nameWinner->setStyleSheet("color: darkred; background-color: rgba(255,255,255,200); padding: 5px; border-radius: 5px;");
    nameWinner->setAlignment(Qt::AlignCenter);
    nameWinner->resize(300, 50);
    nameWinner->move(width()/2 - 150, height()/2 + 170);
    nameWinner->show();

    QLabel *crownLabel = new QLabel("👑", this);
    crownLabel->setFont(QFont("Arial", 120));
    crownLabel->setAlignment(Qt::AlignCenter);
    crownLabel->setStyleSheet("background: transparent;");
    crownLabel->resize(150, 150);
    crownLabel->move(width()/2 - 130, height()/2 - 300);
    crownLabel->show();
}
